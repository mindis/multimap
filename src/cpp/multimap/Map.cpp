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
#include "multimap/internal/thirdparty/mt.hpp"
#include "multimap/internal/Base64.hpp"

namespace multimap {

namespace {

const std::string FILE_PREFIX = "multimap";
const std::string NAME_OF_LOCK_FILE = FILE_PREFIX + ".lock";
const std::string NAME_OF_PROPERTIES_FILE = FILE_PREFIX + ".properties";

bool isPowerOfTwo(std::size_t value) { return (value & (value - 1)) == 0; }

void checkOptions(const Options& options) {
  const auto bs = "options.block_size";
  mt::check(options.block_size != 0, "%s must be positive.", bs);
  mt::check(isPowerOfTwo(options.block_size), "%s must be a power of two.", bs);
}

void checkVersion(std::uint32_t client_major, std::uint32_t client_minor) {
  mt::check(client_major == MAJOR_VERSION && client_minor == MINOR_VERSION,
            "Version check failed. The Multimap you are trying to open "
            "was created with version %u.%u of the library. Your "
            "installed version is %u.%u which is not compatible.",
            client_major, client_minor, MAJOR_VERSION, MINOR_VERSION);
}

}  // namespace

Map::~Map() {
  if (!shards_.empty()) {
    internal::Shard::Stats stats;
    for (auto& shard : shards_) {
      stats.summarize(shard->close());
    }
    auto properties = stats.toProperties();
    properties["map.major_version"] = std::to_string(MAJOR_VERSION);
    properties["map.minor_version"] = std::to_string(MINOR_VERSION);
    properties["map.num_shards"] = std::to_string(shards_.size());

    const auto file = directory_lock_guard_.path() / NAME_OF_PROPERTIES_FILE;
    mt::writePropertiesToFile(properties, file.string());
  }
}

Map::Map(const boost::filesystem::path& directory, const Options& options) {
  open(directory, options);
}

void Map::open(const boost::filesystem::path& directory,
               const Options& options) {
  checkOptions(options);
  const auto abs_dir = boost::filesystem::absolute(directory);
  mt::check(boost::filesystem::is_directory(abs_dir),
            "The path '%s' does not refer to a directory.", abs_dir.c_str());

  directory_lock_guard_.lock(abs_dir, NAME_OF_LOCK_FILE);
  const auto properties_file = abs_dir / NAME_OF_PROPERTIES_FILE;
  const auto map_exists = boost::filesystem::is_regular_file(properties_file);

  if (map_exists) {
    mt::check(!options.error_if_exists,
              "A Multimap in '%s' does already exist.", abs_dir.c_str());

    const auto props = mt::readPropertiesFromFile(properties_file.string());
    const auto major_version = std::stoul(props.at("map.major_version"));
    const auto minor_version = std::stoul(props.at("map.minor_version"));
    checkVersion(major_version, minor_version);

    shards_.resize(std::stoul(props.at("map.num_shards")));

  } else {
    mt::check(options.create_if_missing, "No Multimap found in '%s'.",
              abs_dir.c_str());

    shards_.resize(mt::nextPrime(options.num_shards));
  }

  const auto prefix = abs_dir / FILE_PREFIX;
  for (std::size_t i = 0; i != shards_.size(); ++i) {
    const auto prefix_i = prefix.string() + '.' + std::to_string(i);
    shards_[i].reset(new internal::Shard(prefix_i, options.block_size));
  }
}

void Map::put(const Bytes& key, const Bytes& value) {
  getShard(key).put(key, value);
}

Map::ListIterator Map::get(const Bytes& key) const {
  return getShard(key).get(key);
}

Map::MutableListIterator Map::getMutable(const Bytes& key) {
  return getShard(key).getMutable(key);
}

bool Map::contains(const Bytes& key) const {
  return getShard(key).contains(key);
}

std::size_t Map::remove(const Bytes& key) { return getShard(key).remove(key); }

std::size_t Map::removeAll(const Bytes& key, BytesPredicate predicate) {
  return getShard(key).removeAll(key, predicate);
}

std::size_t Map::removeAllEqual(const Bytes& key, const Bytes& value) {
  return getShard(key).removeAllEqual(key, value);
}

bool Map::removeFirst(const Bytes& key, BytesPredicate predicate) {
  return getShard(key).removeFirst(key, predicate);
}

bool Map::removeFirstEqual(const Bytes& key, const Bytes& value) {
  return getShard(key).removeFirstEqual(key, value);
}

std::size_t Map::replaceAll(const Bytes& key, BytesFunction function) {
  return getShard(key).replaceAll(key, function);
}

std::size_t Map::replaceAllEqual(const Bytes& key, const Bytes& old_value,
                                 const Bytes& new_value) {
  return getShard(key).replaceAllEqual(key, old_value, new_value);
}

bool Map::replaceFirst(const Bytes& key, BytesFunction function) {
  return getShard(key).replaceFirst(key, function);
}

bool Map::replaceFirstEqual(const Bytes& key, const Bytes& old_value,
                            const Bytes& new_value) {
  return getShard(key).replaceFirstEqual(key, old_value, new_value);
}

void Map::forEachKey(BytesProcedure procedure) const {
  MT_REQUIRE_FALSE(shards_.empty());
  for (const auto& shard : shards_) {
    shard->forEachKey(procedure);
  }
}

void Map::forEachValue(const Bytes& key, BytesProcedure procedure) const {
  getShard(key).forEachValue(key, procedure);
}

void Map::forEachValue(const Bytes& key, BytesPredicate predicate) const {
  getShard(key).forEachValue(key, predicate);
}

void Map::forEachEntry(EntryProcedure procedure) const {
  MT_REQUIRE_FALSE(shards_.empty());
  for (const auto& shard : shards_) {
    shard->forEachEntry(procedure);
  }
}

std::map<std::string, std::string> Map::getProperties() const {
  MT_REQUIRE_FALSE(shards_.empty());

  internal::Shard::Stats stats;
  for (const auto& shard : shards_) {
    stats.summarize(shard->getStats());
  }
  auto properties = stats.toProperties();
  properties["map.major_version"] = std::to_string(MAJOR_VERSION);
  properties["map.minor_version"] = std::to_string(MINOR_VERSION);
  properties["map.num_shards"] = std::to_string(shards_.size());
  return properties;
}

std::size_t Map::max_key_size() const {
  MT_REQUIRE_FALSE(shards_.empty());
  return shards_.front()->max_key_size();
}

std::size_t Map::max_value_size() const {
  MT_REQUIRE_FALSE(shards_.empty());
  return shards_.front()->max_value_size();
}

void Map::importFromBase64(const boost::filesystem::path& directory,
                           const boost::filesystem::path& source) {
  Options options;
  options.error_if_exists = false;
  options.create_if_missing = false;
  Map::importFromBase64(directory, source, options);
}

void Map::importFromBase64(const boost::filesystem::path& directory,
                           const boost::filesystem::path& source,
                           const Options& options) {
  Map map(directory, options);

  const auto import_file = [&map](const boost::filesystem::path& filepath) {
    mt::check(boost::filesystem::is_regular_file(filepath),
              "'%s' is not a regular file.", filepath.c_str());
    std::ifstream ifs(filepath.string());
    mt::check(ifs, "Could not open '%s'.", filepath.c_str());

    std::string key_as_base64;
    std::string key_as_binary;
    std::string value_as_base64;
    std::string value_as_binary;
    if (ifs >> key_as_base64) {
      internal::Base64::decode(key_as_base64, &key_as_binary);
      while (ifs) {
        switch (ifs.peek()) {
          case '\n':
          case '\r':
            ifs >> key_as_base64;
            internal::Base64::decode(key_as_base64, &key_as_binary);
            break;
          case '\f':
          case '\t':
          case '\v':
          case ' ':
            ifs.ignore();
            break;
          default:
            ifs >> value_as_base64;
            internal::Base64::decode(value_as_base64, &value_as_binary);
            map.put(key_as_binary, value_as_binary);
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
      if (is_hidden(iter->path())) continue;
      import_file(iter->path());
      ++iter;
    }
  } else {
    import_file(source);
  }
}

void Map::exportToBase64(const boost::filesystem::path& directory,
                         const boost::filesystem::path& file) {
  Map::exportToBase64(directory, file, BytesCompare());
}

void Map::exportToBase64(const boost::filesystem::path& directory,
                         const boost::filesystem::path& file,
                         BytesCompare compare) {
  Map(directory, Options()).exportToBase64(file, compare);
}

void Map::optimize(const boost::filesystem::path& from,
                   const boost::filesystem::path& to, const Options& options) {
  const auto abs_from = boost::filesystem::absolute(from);
  mt::check(boost::filesystem::is_directory(abs_from),
            "The path '%s' does not refer to a directory.", abs_from.c_str());

  internal::System::DirectoryLockGuard lock(abs_from, NAME_OF_LOCK_FILE);
  const auto properties_file = abs_from / NAME_OF_PROPERTIES_FILE;
  const auto map_exists = boost::filesystem::is_regular_file(properties_file);
  mt::check(map_exists, "No Multimap found in '%s'.", abs_from.c_str());

  const auto props = mt::readPropertiesFromFile(properties_file.string());
  const auto major_version = std::stoul(props.at("map.major_version"));
  const auto minor_version = std::stoul(props.at("map.minor_version"));
  checkVersion(major_version, minor_version);

  Options new_options = options;
  new_options.error_if_exists = true;
  new_options.create_if_missing = true;
  if (options.block_size == 0) {
    new_options.block_size = std::stoul(props.at("store.block_size"));
  }
  const auto old_num_shards = std::stoul(props.at("map.num_shards"));
  if (options.num_shards == 0) {
    new_options.num_shards = old_num_shards;
  }

  Map new_map(to, new_options);
  const auto prefix = abs_from / FILE_PREFIX;
  for (std::size_t i = 0; i != old_num_shards; ++i) {
    internal::Shard shard(prefix.string() + '.' + std::to_string(i));
    if (options.bytes_compare) {
      std::vector<std::string> sorted_values;
      // TODO Test if reusing sorted_values makes any difference.
      shard.forEachEntry([&new_map, &options, &sorted_values](
          const Bytes& key, ListIterator&& iter) {
        sorted_values.clear();
        sorted_values.reserve(iter.num_values());
        for (iter.seekToFirst(); iter.hasValue(); iter.next()) {
          sorted_values.push_back(iter.getValue().toString());
        }
        std::sort(sorted_values.begin(), sorted_values.end(),
                  options.bytes_compare);
        for (const auto& value : sorted_values) {
          new_map.put(key, value);
        }
      });
    } else {
      shard.forEachEntry([&new_map](const Bytes& key, ListIterator&& iter) {
        for (iter.seekToFirst(); iter.hasValue(); iter.next()) {
          new_map.put(key, iter.getValue());
        }
      });
    }
  }
}

internal::Shard& Map::getShard(const Bytes& key) {
  MT_REQUIRE_FALSE(shards_.empty());
  return *shards_[mt::fnv1aHash32(key.data(), key.size()) % shards_.size()];
}

const internal::Shard& Map::getShard(const Bytes& key) const {
  MT_REQUIRE_FALSE(shards_.empty());
  return *shards_[mt::fnv1aHash32(key.data(), key.size()) % shards_.size()];
}

void Map::exportToBase64(const boost::filesystem::path& file,
                         BytesCompare compare) {
  std::ofstream ofs(file.string());
  mt::check(ofs, "Could not create '%s'.", file.c_str());

  std::string key_as_base64;
  std::string value_as_base64;
  std::vector<std::string> sorted_values;
  if (compare) {
    forEachEntry([&](const Bytes& key, ListIterator&& iter) {
      MT_ASSERT_NE(iter.num_values(), 0);

      // Sort values
      sorted_values.clear();
      sorted_values.reserve(iter.num_values());
      for (iter.seekToFirst(); iter.hasValue(); iter.next()) {
        sorted_values.push_back(iter.getValue().toString());
      }
      std::sort(sorted_values.begin(), sorted_values.end(), compare);

      // Write as Base64
      auto it = sorted_values.begin();
      if (it != sorted_values.end()) {
        internal::Base64::encode(key, &key_as_base64);
        internal::Base64::encode(*it, &value_as_base64);
        ofs << key_as_base64 << ' ' << value_as_base64;
        ++it;
      }
      while (it != sorted_values.end()) {
        internal::Base64::encode(*it, &value_as_base64);
        ofs << ' ' << value_as_base64;
        ++it;
      }
      ofs << '\n';
    });
  } else {
    forEachEntry([&](const Bytes& key, ListIterator&& iter) {
      MT_ASSERT_NE(iter.num_values(), 0);

      // Write as Base64
      iter.seekToFirst();
      if (iter.hasValue()) {
        internal::Base64::encode(key, &key_as_base64);
        internal::Base64::encode(iter.getValue(), &value_as_base64);
        ofs << key_as_base64 << ' ' << value_as_base64;
        iter.next();
      }
      while (iter.hasValue()) {
        internal::Base64::encode(iter.getValue(), &value_as_base64);
        ofs << ' ' << value_as_base64;
        iter.next();
      }
      ofs << '\n';
    });
  }
}

}  // namespace multimap
