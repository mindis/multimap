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
#include <boost/filesystem/operations.hpp>

namespace multimap {

namespace {

void checkOptions(const Options& options) {
  mt::check(options.block_size != 0, "Options::block_size must be positive");
  mt::check(mt::isPowerOfTwo(options.block_size),
            "Options::block_size must be a power of two");
}

internal::Shard& getShard(
    const std::vector<std::unique_ptr<internal::Shard> >& shards,
    const Bytes& key) {
  MT_REQUIRE_FALSE(shards.empty());
  return *shards[mt::fnv1aHash(key.data(), key.size()) % shards.size()];
}

std::size_t removeValues(
    const std::vector<std::unique_ptr<internal::Shard> >& shards,
    const Bytes& key, Callables::Predicate predicate,
    bool exit_on_first_success) {
  std::size_t num_removed = 0;
  auto iter = Map::MutableListIterator(getShard(shards, key).getUnique(key));
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
    const std::vector<std::unique_ptr<internal::Shard> >& shards,
    const Bytes& key, Callables::Function function,
    bool exit_on_first_success) {
  std::vector<std::string> replaced_values;
  auto iter = Map::MutableListIterator(getShard(shards, key).getUnique(key));
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
  return internal::Shard::Limits::getMaxKeySize();
}

std::size_t Map::Limits::getMaxValueSize() {
  return internal::Shard::Limits::getMaxValueSize();
}

Map::Map(const boost::filesystem::path& directory, const Options& options) {
  checkOptions(options);
  const auto abs_dir = boost::filesystem::absolute(directory);
  mt::check(boost::filesystem::is_directory(abs_dir),
            "The path '%s' does not refer to a directory.", abs_dir.c_str());

  lock_ = mt::DirectoryLockGuard(abs_dir, internal::getNameOfLockFile());
  const auto id_file = abs_dir / internal::getNameOfIdFile();

  if (boost::filesystem::is_regular_file(id_file)) {
    mt::check(!options.error_if_exists,
              "A Multimap in '%s' does already exist.", abs_dir.c_str());

    const auto id = internal::Id::readFromFile(id_file);
    internal::checkVersion(id.major_version, id.minor_version);
    shards_.resize(id.num_shards);

  } else {
    mt::check(options.create_if_missing, "No Multimap found in '%s'.",
              abs_dir.c_str());

    shards_.resize(mt::nextPrime(options.num_shards));
  }

  internal::Shard::Options shard_options;
  shard_options.block_size = options.block_size;
  shard_options.buffer_size = options.buffer_size;
  shard_options.create_if_missing = options.create_if_missing;
  shard_options.error_if_exists = options.error_if_exists;
  shard_options.readonly = options.readonly;

  for (std::size_t i = 0; i != shards_.size(); ++i) {
    const auto prefix = abs_dir / internal::getShardPrefix(i);
    shards_[i].reset(new internal::Shard(prefix, shard_options));
  }
}

Map::~Map() {
  if (!shards_.empty()) {
    internal::Id id;
    id.num_shards = shards_.size();
    id.writeToFile(lock_.directory() / internal::getNameOfIdFile());
  }
}

void Map::put(const Bytes& key, const Bytes& value) {
  getShard(shards_, key).getUniqueOrCreate(key).add(value);
}

Map::ListIterator Map::get(const Bytes& key) const {
  return Map::ListIterator(getShard(shards_, key).getShared(key));
}

Map::MutableListIterator Map::getMutable(const Bytes& key) {
  return Map::MutableListIterator(getShard(shards_, key).getUnique(key));
}

bool Map::contains(const Bytes& key) const {
  if (auto list = getShard(shards_, key).getShared(key)) {
    return list.size() != 0;
  }
  return false;
}

std::size_t Map::remove(const Bytes& key) {
  if (auto list = getShard(shards_, key).getUnique(key)) {
    return list.clear();
  }
  return 0;
}

std::size_t Map::removeAll(const Bytes& key, Callables::Predicate predicate) {
  return removeValues(shards_, key, predicate, false);
}

std::size_t Map::removeAllEqual(const Bytes& key, const Bytes& value) {
  return removeAll(key, [&value](const Bytes& current_value) {
    return current_value == value;
  });
}

bool Map::removeFirst(const Bytes& key, Callables::Predicate predicate) {
  return removeValues(shards_, key, predicate, true);
}

bool Map::removeFirstEqual(const Bytes& key, const Bytes& value) {
  return removeFirst(key, [&value](const Bytes& current_value) {
    return current_value == value;
  });
}

std::size_t Map::replaceAll(const Bytes& key, Callables::Function function) {
  return replaceValues(shards_, key, function, false);
}

std::size_t Map::replaceAllEqual(const Bytes& key, const Bytes& old_value,
                                 const Bytes& new_value) {
  return replaceAll(key, [&old_value, &new_value](const Bytes& current_value)
                             -> std::string {
    return (current_value == old_value) ? new_value.toString() : std::string();
  });
}

bool Map::replaceFirst(const Bytes& key, Callables::Function function) {
  return replaceValues(shards_, key, function, true);
}

bool Map::replaceFirstEqual(const Bytes& key, const Bytes& old_value,
                            const Bytes& new_value) {
  return replaceFirst(key,
                      [&old_value, &new_value](const Bytes& current_value) {
    return (current_value == old_value) ? new_value.toString() : std::string();
  });
}

void Map::forEachKey(Callables::Procedure action) const {
  MT_REQUIRE_FALSE(shards_.empty());
  for (const auto& shard : shards_) {
    shard->forEachKey(action);
  }
}

void Map::forEachValue(const Bytes& key, Callables::Procedure action) const {
  auto iter = ListIterator(getShard(shards_, key).getShared(key));
  while (iter.hasNext()) {
    action(iter.next());
  }
}

void Map::forEachValue(const Bytes& key, Callables::Predicate action) const {
  auto iter = ListIterator(getShard(shards_, key).getShared(key));
  while (iter.hasNext()) {
    if (!action(iter.next())) {
      break;
    }
  }
}

void Map::forEachEntry(Callables::BinaryProcedure action) const {
  MT_REQUIRE_FALSE(shards_.empty());
  for (const auto& shard : shards_) {
    shard->forEachEntry(
        [action, this](const Bytes& key, internal::SharedList&& list) {
          action(key, ListIterator(std::move(list)));
        });
  }
}

std::vector<Map::Stats> Map::getStats() const {
  std::vector<Stats> stats;
  for (const auto& shard : shards_) {
    stats.push_back(shard->getStats());
  }
  return stats;
}

Map::Stats Map::getTotalStats() const {
  return internal::Shard::Stats::total(getStats());
}

namespace internal {

Id Id::readFromFile(const boost::filesystem::path& file) {
  Id id;
  const auto stream = mt::open(file, "r");
  mt::check(stream.get(), "Could not open '%s'", file.c_str());
  mt::read(stream.get(), &id, sizeof id);
  return id;
}

void Id::writeToFile(const boost::filesystem::path& file) const {
  const auto stream = mt::open(file, "w");
  mt::check(stream.get(), "Could not create '%s'", file.c_str());
  mt::write(stream.get(), this, sizeof *this);
}

const std::string getFilePrefix() { return "multimap"; }

const std::string getNameOfIdFile() { return getFilePrefix() + ".id"; }

const std::string getNameOfLockFile() { return getFilePrefix() + ".lock"; }

const std::string getShardPrefix(std::size_t index) {
  return getFilePrefix() + '.' + std::to_string(index);
}

const std::string getNameOfKeysFile(std::size_t index) {
  return Shard::getNameOfKeysFile(getShardPrefix(index));
}

const std::string getNameOfStatsFile(std::size_t index) {
  return Shard::getNameOfStatsFile(getShardPrefix(index));
}

const std::string getNameOfValuesFile(std::size_t index) {
  return Shard::getNameOfValuesFile(getShardPrefix(index));
}

void checkVersion(std::uint64_t major_version, std::uint64_t minor_version) {
  mt::check(major_version == MAJOR_VERSION && minor_version == MINOR_VERSION,
            "Version check failed. The Multimap you are trying to open "
            "was created with version %u.%u of the library. Your "
            "installed version is %u.%u which is not compatible.",
            major_version, minor_version, MAJOR_VERSION, MINOR_VERSION);
}

} // namespace internal
} // namespace multimap
