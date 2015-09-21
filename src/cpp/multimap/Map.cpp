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

#include <fstream>
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem/operations.hpp>
#include "multimap/internal/thirdparty/mt.hpp"
#include "multimap/internal/Base64.hpp"

namespace multimap {

namespace {

const char* NAME_OF_LOCK_FILE = "multimap.lock";
const char* NAME_OF_PROPERTIES_FILE = "multimap.properties";

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

Map::Map() {}

Map::~Map() {
  if (!shards_.empty()) {
    for (auto& shard : shards_) {
      shard->flush();
    }
    // Flushing ensures that we get the final properties.
    const auto fname = directory_lock_guard_.path() / NAME_OF_PROPERTIES_FILE;
    mt::writePropertiesToFile(getProperties(), fname.c_str());
  }
}

Map::Map(const boost::filesystem::path& directory, const Options& options) {
  open(directory, options);
}

void Map::open(const boost::filesystem::path& directory,
               const Options& options) {
  checkOptions(options);
  const auto absolute_directory = boost::filesystem::absolute(directory);
  mt::check(boost::filesystem::is_directory(absolute_directory),
            "The path '%s' does not refer to a directory.",
            absolute_directory.c_str());

  directory_lock_guard_.lock(absolute_directory, NAME_OF_LOCK_FILE);
  const auto path_to_props = absolute_directory / NAME_OF_PROPERTIES_FILE;

  //  options_ = options;
  if (boost::filesystem::is_regular_file(path_to_props)) {
    mt::check(!options.error_if_exists,
              "A Multimap in '%s' does already exist.",
              absolute_directory.c_str());

    // TODO More sanity checks

    const auto properties = mt::readPropertiesFromFile(path_to_props.c_str());
    const auto num_shards = std::stoul(properties.at("map.num_shards"));

    shards_.resize(num_shards);
    auto prefix = absolute_directory / "multimap";
    for (std::size_t i = 0; i != shards_.size(); ++i) {
      const auto prefix_i = prefix.string() + '.' + std::to_string(i);
      shards_[i].reset(new internal::Shard(prefix_i));
    }

  } else {
    mt::check(options.create_if_missing, "No Multimap found in '%s'.",
              absolute_directory.c_str());

    shards_.resize(mt::nextPrime(options.num_shards));
    auto prefix = absolute_directory / "multimap";
    for (std::size_t i = 0; i != shards_.size(); ++i) {
      const auto prefix_i = prefix.string() + '.' + std::to_string(i);
      shards_[i].reset(new internal::Shard(prefix_i, options.block_size));
    }
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

std::size_t Map::removeAll(const Bytes& key,
                           Callables::BytesPredicate predicate) {
  return getShard(key).removeAll(key, predicate);
}

std::size_t Map::removeAllEqual(const Bytes& key, const Bytes& value) {
  return getShard(key).removeAllEqual(key, value);
}

bool Map::removeFirst(const Bytes& key, Callables::BytesPredicate predicate) {
  return getShard(key).removeFirst(key, predicate);
}

bool Map::removeFirstEqual(const Bytes& key, const Bytes& value) {
  return getShard(key).removeFirstEqual(key, value);
}

std::size_t Map::replaceAll(const Bytes& key,
                            Callables::BytesFunction function) {
  return getShard(key).replaceAll(key, function);
}

std::size_t Map::replaceAllEqual(const Bytes& key, const Bytes& old_value,
                                 const Bytes& new_value) {
  return getShard(key).replaceAllEqual(key, old_value, new_value);
}

bool Map::replaceFirst(const Bytes& key, Callables::BytesFunction function) {
  return getShard(key).replaceFirst(key, function);
}

bool Map::replaceFirstEqual(const Bytes& key, const Bytes& old_value,
                            const Bytes& new_value) {
  return getShard(key).replaceFirstEqual(key, old_value, new_value);
}

void Map::forEachKey(Callables::BytesProcedure procedure) const {
  MT_REQUIRE_FALSE(shards_.empty());
  for (const auto& shard : shards_) {
    shard->forEachKey(procedure);
  }
}

void Map::forEachValue(const Bytes& key,
                       Callables::BytesProcedure procedure) const {
  getShard(key).forEachValue(key, procedure);
}

void Map::forEachValue(const Bytes& key,
                       Callables::BytesPredicate predicate) const {
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
    stats.combine(shard->getStats());
  }
  auto properties = stats.toMap();
  properties["map.major_version"] = std::to_string(MAJOR_VERSION);
  properties["map.minor_version"] = std::to_string(MINOR_VERSION);
  properties["map.num_shards"] = std::to_string(shards_.size());
  return properties;
}

std::size_t Map::max_key_size() const {
  return shards_.empty() ? 0 : shards_.front()->max_key_size();
}

std::size_t Map::max_value_size() const {
  return shards_.empty() ? 0 : shards_.front()->max_value_size();
}

void Map::importFromBase64(const boost::filesystem::path& directory,
                           const boost::filesystem::path& file) {
  Map::importFromBase64(directory, file, false);
}

void Map::importFromBase64(const boost::filesystem::path& directory,
                           const boost::filesystem::path& file,
                           bool create_if_missing) {
  Map::importFromBase64(directory, file, create_if_missing, -1);
}

void Map::importFromBase64(const boost::filesystem::path& directory,
                           const boost::filesystem::path& file,
                           bool create_if_missing, std::size_t block_size) {
  Options options;
  options.create_if_missing = create_if_missing;
  if (block_size != static_cast<std::size_t>(-1)) {
    options.block_size = block_size;
  }
  Map map(directory, options);
  map.importFromBase64(file);
}

void Map::exportToBase64(const boost::filesystem::path& directory,
                         const boost::filesystem::path& file) {
  Map::exportToBase64(directory, file, Callables::BytesCompare());
}

void Map::exportToBase64(const boost::filesystem::path& directory,
                         const boost::filesystem::path& file,
                         Callables::BytesCompare compare) {
  Map map(directory, Options());
  map.exportToBase64(file, compare);
}

void Map::optimize(const boost::filesystem::path& directory) {
  Map::optimize(directory, -1, BytesCompare());
}

void Map::optimize(const boost::filesystem::path& directory,
                   BytesCompare compare) {
  Map::optimize(directory, -1, compare);
}

void Map::optimize(const boost::filesystem::path& directory,
                   std::size_t new_block_size) {
  Map::optimize(directory, new_block_size, BytesCompare());
}

void Map::optimize(const boost::filesystem::path& directory,
                   std::size_t new_block_size, BytesCompare compare) {
  Map map(directory, Options());
  map.optimize(new_block_size, compare);
}

internal::Shard& Map::getShard(const Bytes& key) {
  MT_REQUIRE_FALSE(shards_.empty());
  return *shards_[mt::fnv1aHash32(key.data(), key.size()) % shards_.size()];
}

const internal::Shard& Map::getShard(const Bytes& key) const {
  MT_REQUIRE_FALSE(shards_.empty());
  return *shards_[mt::fnv1aHash32(key.data(), key.size()) % shards_.size()];
}

void Map::importFromBase64(const boost::filesystem::path& file) {
  std::ifstream ifs(file.string());
  mt::check(ifs, "Cannot open '%s'.", file.c_str());

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
          put(key_as_binary, value_as_binary);
      }
    }
  }
}

void Map::exportToBase64(const boost::filesystem::path& file,
                         BytesCompare compare) {
  std::ofstream ofs(file.string());
  mt::check(ofs, "Cannot create '%s'.", file.c_str());

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

void Map::optimize(std::size_t new_block_size, BytesCompare compare) {
  // No locking since a user cannot call this method directly, because it's
  // private. It can only be called indirectly via the static version of this
  // operation. Hence, this method has always exclusive access to the map.
  for (auto& shard : shards_) {
    shard->optimize(compare, new_block_size);
  }
}

// TODO Remove
void optimize(const boost::filesystem::path& from,
              const boost::filesystem::path& to, std::size_t new_block_size,
              Map::BytesCompare compare) {
  Map map_in(from, Options());

  Options options;
  options.error_if_exists = true;
  options.create_if_missing = true;
  options.block_size = std::stoul(map_in.getProperties()["store.block_size"]);
  if (new_block_size != static_cast<std::size_t>(-1)) {
    options.block_size = new_block_size;
  }
  Map map_out(to, options);

  if (compare) {
    map_in.forEachEntry([&](const Bytes& key, Map::ListIterator&& iter) {
      std::vector<std::string> values;
      values.reserve(iter.num_values());
      for (iter.seekToFirst(); iter.hasValue(); iter.next()) {
        values.push_back(iter.getValue().toString());
      }
      std::sort(values.begin(), values.end(), compare);
      for (const auto& value : values) {
        map_out.put(key, value);
      }
    });
  } else {
    map_in.forEachEntry([&](const Bytes& key, Map::ListIterator&& iter) {
      for (iter.seekToFirst(); iter.hasValue(); iter.next()) {
        map_out.put(key, iter.getValue());
      }
    });
  }
}

}  // namespace multimap
