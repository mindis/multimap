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
#include "multimap/thirdparty/mt.hpp"

namespace multimap {

namespace {

const std::string FILE_PREFIX = "multimap";
const std::string NAME_OF_LOCK_FILE = FILE_PREFIX + ".lock";
const std::string NAME_OF_ID_FILE = FILE_PREFIX + ".id";

struct Id {
  std::uint64_t block_size = 0;
  std::uint64_t num_tables = 0;
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

internal::Shard& selectShard(
    const std::vector<std::unique_ptr<internal::Shard> >& shards,
    const Bytes& key) {
  MT_REQUIRE_FALSE(shards.empty());
  return *shards[mt::fnv1aHash32(key.data(), key.size()) % shards.size()];
}

} // namespace

std::size_t Map::Limits::max_key_size() {
  return internal::Shard::Limits::max_key_size();
}

std::size_t Map::Limits::max_value_size() {
  return internal::Shard::Limits::max_value_size();
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

    shards_.resize(id.num_tables);

  } else {
    mt::check(options.create_if_missing, "No Multimap found in '%s'.",
              abs_dir.c_str());

    shards_.resize(mt::nextPrime(options.num_shards));
  }

  internal::Shard::Options shard_opts;
  shard_opts.block_size = options.block_size;
  shard_opts.create_if_missing = options.create_if_missing;
  shard_opts.error_if_exists = options.error_if_exists;

  const auto prefix = abs_dir / FILE_PREFIX;
  for (std::size_t i = 0; i != shards_.size(); ++i) {
    const auto prefix_i = prefix.string() + '.' + std::to_string(i);
    shards_[i].reset(new internal::Shard(prefix_i, shard_opts));
  }
}

Map::~Map() {
  if (!shards_.empty()) {
    Id id;
    id.num_tables = shards_.size();
    id.block_size = shards_.front()->getBlockSize();
    writeIdToFile(id, directory_lock_guard_.path() / NAME_OF_ID_FILE);
  }
}

void Map::put(const Bytes& key, const Bytes& value) {
  selectShard(shards_, key).put(key, value);
}

Map::ListIterator Map::get(const Bytes& key) const {
  return selectShard(shards_, key).getShared(key);
}

Map::MutableListIterator Map::getMutable(const Bytes& key) {
  return selectShard(shards_, key).getUnique(key);
}

bool Map::contains(const Bytes& key) const {
  return selectShard(shards_, key).contains(key);
}

std::size_t Map::remove(const Bytes& key) {
  return selectShard(shards_, key).remove(key);
}

std::size_t Map::removeAll(const Bytes& key, Callables::Predicate predicate) {
  return selectShard(shards_, key).removeAll(key, predicate);
}

std::size_t Map::removeAllEqual(const Bytes& key, const Bytes& value) {
  return selectShard(shards_, key).removeAllEqual(key, value);
}

bool Map::removeFirst(const Bytes& key, Callables::Predicate predicate) {
  return selectShard(shards_, key).removeFirst(key, predicate);
}

bool Map::removeFirstEqual(const Bytes& key, const Bytes& value) {
  return selectShard(shards_, key).removeFirstEqual(key, value);
}

std::size_t Map::replaceAll(const Bytes& key, Callables::Function function) {
  return selectShard(shards_, key).replaceAll(key, function);
}

std::size_t Map::replaceAllEqual(const Bytes& key, const Bytes& old_value,
                                 const Bytes& new_value) {
  return selectShard(shards_, key).replaceAllEqual(key, old_value, new_value);
}

bool Map::replaceFirst(const Bytes& key, Callables::Function function) {
  return selectShard(shards_, key).replaceFirst(key, function);
}

bool Map::replaceFirstEqual(const Bytes& key, const Bytes& old_value,
                            const Bytes& new_value) {
  return selectShard(shards_, key).replaceFirstEqual(key, old_value, new_value);
}

void Map::forEachKey(Callables::Procedure action) const {
  MT_REQUIRE_FALSE(shards_.empty());
  for (const auto& shard : shards_) {
    shard->forEachKey(action);
  }
}

void Map::forEachValue(const Bytes& key, Callables::Procedure action) const {
  selectShard(shards_, key).forEachValue(key, action);
}

void Map::forEachValue(const Bytes& key, Callables::Predicate action) const {
  selectShard(shards_, key).forEachValue(key, action);
}

void Map::forEachEntry(Callables::Procedure2 action) const {
  MT_REQUIRE_FALSE(shards_.empty());
  for (const auto& shard : shards_) {
    shard->forEachEntry(action);
  }
}

std::map<std::string, std::string> Map::getProperties() const {
  MT_REQUIRE_FALSE(shards_.empty());

  internal::Shard::Stats stats;
  for (const auto& shard : shards_) {
    stats.combine(shard->getStats());
  }
  auto properties = stats.toProperties();
  properties["map.num_shards"] = std::to_string(shards_.size());
  properties["map.version_major"] = std::to_string(VERSION_MAJOR);
  properties["map.version_minor"] = std::to_string(VERSION_MINOR);
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

  Options new_options = options;
  new_options.error_if_exists = true;
  new_options.create_if_missing = true;
  if (options.block_size == 0) {
    new_options.block_size = id.block_size;
  }
  if (options.num_shards == 0) {
    new_options.num_shards = id.num_tables;
  }

  Map new_map(target, new_options);
  const auto prefix = abs_source / FILE_PREFIX;
  for (std::size_t i = 0; i != id.num_tables; ++i) {
    internal::Shard shard(prefix.string() + '.' + std::to_string(i),
                          internal::Shard::Options());
    if (options.compare_bytes) {
      std::vector<std::string> sorted_values;
      // TODO Test if reusing sorted_values makes any difference.
      shard.forEachEntry([&new_map, &options, &sorted_values](
          const Bytes& key, ListIterator&& iter) {
        sorted_values.clear();
        sorted_values.reserve(iter.available());
        while (iter.hasNext()) {
          sorted_values.push_back(iter.next().toString());
        }
        std::sort(sorted_values.begin(), sorted_values.end(),
                  options.compare_bytes);
        for (const auto& value : sorted_values) {
          new_map.put(key, value);
        }
      });
    } else {
      shard.forEachEntry([&new_map](const Bytes& key, ListIterator&& iter) {
        while (iter.hasNext()) {
          new_map.put(key, iter.next());
        }
      });
    }
  }
}

} // namespace multimap
