// This file is part of the Multimap library.  http://multimap.io
//
// Copyright (C) 2015  Martin Trenkmann
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "multimap/Map.hpp"

#include <algorithm>
#include <fstream>
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem/operations.hpp>
#include "multimap/internal/Base64.hpp"
#include "multimap/thirdparty/mt/mt.hpp"

namespace multimap {

namespace {

const std::string FILE_PREFIX = "multimap";
const std::string NAME_OF_LOCK_FILE = FILE_PREFIX + ".lock";
const std::string NAME_OF_ID_FILE = FILE_PREFIX + ".id";

struct Id {
  std::uint64_t checksum = 0;
  std::uint64_t num_shards = 0;
  std::uint64_t version_major = VERSION_MAJOR;
  std::uint64_t version_minor = VERSION_MINOR;
};

Id readIdFromFile(const boost::filesystem::path& filepath) {
  Id id;
  std::ifstream(filepath.string())
      .read(reinterpret_cast<char*>(&id), sizeof id);
  return id;
}

void writeIdToFile(const Id& id, const boost::filesystem::path& filepath) {
  std::ofstream(filepath.string())
      .write(reinterpret_cast<const char*>(&id), sizeof id);
}

void checkOptions(const Options& options) {
  mt::check(options.block_size != 0, "options.block_size must be positive");
  mt::check(mt::isPowerOfTwo(options.block_size),
            "options.block_size must be a power of two");
}

void checkVersion(std::uint32_t client_major, std::uint32_t client_minor) {
  mt::check(client_major == VERSION_MAJOR && client_minor == VERSION_MINOR,
            "Version check failed. The Multimap you are trying to open "
            "was created with version %u.%u of the library. Your "
            "installed version is %u.%u which is not compatible.",
            client_major, client_minor, VERSION_MAJOR, VERSION_MINOR);
}

internal::Table& getTable(
    const std::vector<std::unique_ptr<internal::Table> >& tables,
    const Bytes& key) {
  MT_REQUIRE_FALSE(tables.empty());
  // TODO Replace by xxhash?
  return *tables[mt::fnv1aHash32(key.data(), key.size()) % tables.size()];
}

std::size_t removeValues(
    const std::vector<std::unique_ptr<internal::Table> >& tables,
    const Bytes& key, Callables::Predicate predicate,
    bool exit_on_first_success) {
  std::size_t num_removed = 0;
  auto iter = Map::MutableListIterator(getTable(tables, key).getUnique(key));
  while (iter.hasNext()) {
    if (predicate(iter.next())) {
      iter.remove();
      ++num_removed;
      if (exit_on_first_success) {
        break;
      }
    }
  }
  return num_removed;
}

std::size_t replaceValues(
    const std::vector<std::unique_ptr<internal::Table> >& tables,
    const Bytes& key, Callables::Function function,
    bool exit_on_first_success) {
  std::vector<std::string> replaced_values;
  auto iter = Map::MutableListIterator(getTable(tables, key).getUnique(key));
  while (iter.hasNext()) {
    auto replaced_value = function(iter.next());
    if (!replaced_value.empty()) {
      replaced_values.push_back(std::move(replaced_value));
      iter.remove();
      if (exit_on_first_success) {
        break;
      }
    }
  }
  auto list = iter.release();
  for (const auto& value : replaced_values) {
    list.add(value);
  }
  return replaced_values.size();
}

} // namespace

std::size_t Map::Limits::getMaxKeySize() {
  return internal::Table::Limits::getMaxKeySize();
}

std::size_t Map::Limits::getMaxValueSize() {
  return internal::Table::Limits::getMaxValueSize();
}

Map::Map(const boost::filesystem::path& directory, const Options& options) {
  checkOptions(options);
  const auto abs_dir = boost::filesystem::absolute(directory);
  mt::check(boost::filesystem::is_directory(abs_dir),
            "The path '%s' does not refer to a directory.", abs_dir.c_str());

  directory_lock_guard_.lock(abs_dir, NAME_OF_LOCK_FILE);
  const auto id_file = abs_dir / NAME_OF_ID_FILE;
  const auto map_exists = boost::filesystem::is_regular_file(id_file);

  if (map_exists) {
    mt::check(!options.error_if_exists,
              "A Multimap in '%s' does already exist.", abs_dir.c_str());

    const auto id = readIdFromFile(id_file);
    checkVersion(id.version_major, id.version_minor);

    tables_.resize(id.num_shards);

  } else {
    mt::check(options.create_if_missing, "No Multimap found in '%s'.",
              abs_dir.c_str());

    tables_.resize(mt::nextPrime(options.num_shards));
  }

  internal::Table::Options table_opts;
  table_opts.block_size = options.block_size;
  table_opts.buffer_size = options.buffer_size;
  table_opts.create_if_missing = options.create_if_missing;
  table_opts.error_if_exists = options.error_if_exists;

  const auto prefix = abs_dir / FILE_PREFIX;
  for (std::size_t i = 0; i != tables_.size(); ++i) {
    const auto prefix_i = prefix.string() + '.' + std::to_string(i);
    tables_[i].reset(new internal::Table(prefix_i, table_opts));
  }
}

Map::~Map() {
  if (!tables_.empty()) {
    Id id;
    id.num_shards = tables_.size();
    writeIdToFile(id, directory_lock_guard_.path() / NAME_OF_ID_FILE);
  }
}

void Map::put(const Bytes& key, const Bytes& value) {
  getTable(tables_, key).getUniqueOrCreate(key).add(value);
}

Map::ListIterator Map::get(const Bytes& key) const {
  return Map::ListIterator(getTable(tables_, key).getShared(key));
}

Map::MutableListIterator Map::getMutable(const Bytes& key) {
  return Map::MutableListIterator(getTable(tables_, key).getUnique(key));
}

bool Map::contains(const Bytes& key) const {
  if (auto list = getTable(tables_, key).getShared(key)) {
    return list.size() != 0;
  }
  return false;
}

std::size_t Map::remove(const Bytes& key) {
  if (auto list = getTable(tables_, key).getUnique(key)) {
    return list.clear();
  }
  return 0;
}

std::size_t Map::removeAll(const Bytes& key, Callables::Predicate predicate) {
  return removeValues(tables_, key, predicate, false);
}

std::size_t Map::removeAllEqual(const Bytes& key, const Bytes& value) {
  return removeAll(key, [&value](const Bytes& current_value) {
    return current_value == value;
  });
}

bool Map::removeFirst(const Bytes& key, Callables::Predicate predicate) {
  return removeValues(tables_, key, predicate, true);
}

bool Map::removeFirstEqual(const Bytes& key, const Bytes& value) {
  return removeFirst(key, [&value](const Bytes& current_value) {
    return current_value == value;
  });
}

std::size_t Map::replaceAll(const Bytes& key, Callables::Function function) {
  return replaceValues(tables_, key, function, false);
}

std::size_t Map::replaceAllEqual(const Bytes& key, const Bytes& old_value,
                                 const Bytes& new_value) {
  return replaceAll(key, [&old_value, &new_value](const Bytes& current_value)
                             -> std::string {
    return (current_value == old_value) ? new_value.toString() : std::string();
  });
}

bool Map::replaceFirst(const Bytes& key, Callables::Function function) {
  return replaceValues(tables_, key, function, true);
}

bool Map::replaceFirstEqual(const Bytes& key, const Bytes& old_value,
                            const Bytes& new_value) {
  return replaceFirst(key,
                      [&old_value, &new_value](const Bytes& current_value) {
    return (current_value == old_value) ? new_value.toString() : std::string();
  });
}

void Map::forEachKey(Callables::Procedure action) const {
  MT_REQUIRE_FALSE(tables_.empty());
  for (const auto& table : tables_) {
    table->forEachKey(action);
  }
}

void Map::forEachValue(const Bytes& key, Callables::Procedure action) const {
  auto iter = ListIterator(getTable(tables_, key).getShared(key));
  while (iter.hasNext()) {
    action(iter.next());
  }
}

void Map::forEachValue(const Bytes& key, Callables::Predicate action) const {
  auto iter = ListIterator(getTable(tables_, key).getShared(key));
  while (iter.hasNext()) {
    if (!action(iter.next())) {
      break;
    }
  }
}

void Map::forEachEntry(Callables::BinaryProcedure action) const {
  MT_REQUIRE_FALSE(tables_.empty());
  for (const auto& table : tables_) {
    table->forEachEntry(
        [action, this](const Bytes& key, internal::SharedList&& list) {
          action(key, ListIterator(std::move(list)));
        });
  }
}

std::map<std::string, std::string> Map::getProperties() const {
  MT_REQUIRE_FALSE(tables_.empty());

  internal::Table::Stats stats;
  for (const auto& table : tables_) {
    stats.combine(table->getStats());
  }
  auto properties = stats.toProperties();
  properties["num_shards"] = std::to_string(tables_.size());
  properties["version_major"] = std::to_string(VERSION_MAJOR);
  properties["version_minor"] = std::to_string(VERSION_MINOR);
  return properties;
}

void Map::importFromBase64(const boost::filesystem::path& target,
                           const boost::filesystem::path& source) {
  Options options;
  options.error_if_exists = false;
  options.create_if_missing = false;
  Map::importFromBase64(target, source, options);
}

void Map::importFromBase64(const boost::filesystem::path& target,
                           const boost::filesystem::path& source,
                           const Options& options) {
  Map map(target, options);

  const auto import_file = [&map](const boost::filesystem::path& filepath) {
    mt::check(boost::filesystem::is_regular_file(filepath),
              "'%s' is not a regular file.", filepath.c_str());
    std::ifstream ifs(filepath.string());
    mt::check(ifs, "Could not open '%s'.", filepath.c_str());

    std::string base64_key;
    std::string base64_value;
    std::string binary_key;
    std::string binary_value;
    if (ifs >> base64_key) {
      internal::Base64::decode(base64_key, &binary_key);
      while (ifs) {
        switch (ifs.peek()) {
          case '\n':
          case '\r':
            ifs >> base64_key;
            internal::Base64::decode(base64_key, &binary_key);
            break;
          case '\f':
          case '\t':
          case '\v':
          case ' ':
            ifs.ignore();
            break;
          default:
            ifs >> base64_value;
            internal::Base64::decode(base64_value, &binary_value);
            map.put(binary_key, binary_value);
        }
      }
    }
  };

  const auto is_hidden = [](const boost::filesystem::path& path) {
    return path.filename().string().front() == '.';
  };

  if (boost::filesystem::is_directory(source)) {
    boost::filesystem::directory_iterator end;
    boost::filesystem::directory_iterator iter(source);
    while (iter != end) {
      if (is_hidden(iter->path()))
        continue;
      import_file(iter->path());
      ++iter;
    }
  } else {
    import_file(source);
  }
}

void Map::exportToBase64(const boost::filesystem::path& source,
                         const boost::filesystem::path& target) {
  Map::exportToBase64(source, target, Callables::Compare());
}

void Map::exportToBase64(const boost::filesystem::path& source,
                         const boost::filesystem::path& target,
                         Callables::Compare compare) {
  Options options;
  options.error_if_exists = false;
  options.create_if_missing = false;
  Map map(source, options);

  std::ofstream ofs(target.string());
  mt::check(ofs, "Could not create '%s'.", target.c_str());

  std::string base64_key;
  std::string base64_value;
  std::vector<std::string> sorted_values;
  if (compare) {
    map.forEachEntry([&](const Bytes& key, ListIterator&& iter) {
      // TODO Test if reusing sorted_values makes any difference.

      // Sort values
      sorted_values.clear();
      sorted_values.reserve(iter.available());
      while (iter.hasNext()) {
        sorted_values.push_back(iter.next().toString());
      }
      std::sort(sorted_values.begin(), sorted_values.end(), compare);

      // Write as Base64
      internal::Base64::encode(key, &base64_key);
      ofs << base64_key;
      for (const auto& value : sorted_values) {
        internal::Base64::encode(value, &base64_value);
        ofs << ' ' << base64_value;
      }
      ofs << '\n';
    });
  } else {
    map.forEachEntry([&](const Bytes& key, ListIterator&& iter) {
      // Write as Base64
      internal::Base64::encode(key, &base64_key);
      ofs << base64_key;
      while (iter.hasNext()) {
        internal::Base64::encode(iter.next(), &base64_value);
        ofs << ' ' << base64_value;
      }
      ofs << '\n';
    });
  }
}

void Map::optimize(const boost::filesystem::path& source,
                   const boost::filesystem::path& target,
                   const Options& options) {
  const auto abs_source = boost::filesystem::absolute(source);
  mt::check(boost::filesystem::is_directory(abs_source),
            "The path '%s' does not refer to a directory.", abs_source.c_str());

  internal::System::DirectoryLockGuard lock(abs_source, NAME_OF_LOCK_FILE);
  const auto id_file = abs_source / NAME_OF_ID_FILE;
  const auto map_exists = boost::filesystem::is_regular_file(id_file);
  mt::check(map_exists, "No Multimap found in '%s'.", abs_source.c_str());

  const auto id = readIdFromFile(id_file);
  checkVersion(id.version_major, id.version_minor);

  // TODO verify id.checksum.

  Map new_map;
  const auto prefix = abs_source / FILE_PREFIX;
  for (std::size_t i = 0; i != id.num_shards; ++i) {
    const auto table_prefix = prefix.string() + '.' + std::to_string(i);
    internal::Table table(table_prefix, internal::Table::Options());

    if (i == 0) {
      Options new_opts = options;
      new_opts.error_if_exists = true;
      new_opts.create_if_missing = true;
      if (new_opts.block_size == 0) {
        new_opts.block_size = table.getBlockSize();
      }
      if (new_opts.num_shards == 0) {
        new_opts.num_shards = id.num_shards;
      }
      new_map = Map(target, new_opts);
    }

    if (options.compare) {
      std::vector<std::string> sorted_values;
      // TODO Test if reusing sorted_values makes any difference.
      table.forEachEntry([&new_map, &options, &sorted_values](
          const Bytes& key, ListIterator&& iter) {
        sorted_values.clear();
        sorted_values.reserve(iter.available());
        while (iter.hasNext()) {
          sorted_values.push_back(iter.next().toString());
        }
        std::sort(sorted_values.begin(), sorted_values.end(),
                  options.compare);
        for (const auto& value : sorted_values) {
          new_map.put(key, value);
        }
      });
    } else {
      table.forEachEntry([&new_map](const Bytes& key, ListIterator&& iter) {
        while (iter.hasNext()) {
          new_map.put(key, iter.next());
        }
      });
    }
  }
}

} // namespace multimap
