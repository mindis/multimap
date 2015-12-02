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

#include <boost/filesystem/operations.hpp>

namespace multimap {

namespace {

void checkOptions(const Options& options) {
  mt::Check::notZero(options.block_size,
                     "Options::block_size must be positive");
  mt::Check::isTrue(mt::isPowerOfTwo(options.block_size),
                    "Options::block_size must be a power of two");
}

internal::Shard& getShard(
    const std::vector<std::unique_ptr<internal::Shard> >& shards,
    const Bytes& key) {
  MT_REQUIRE_FALSE(shards.empty());
  return *shards[mt::fnv1aHash(key.data(), key.size()) % shards.size()];
}

} // namespace

Map::Id Map::Id::readFromDirectory(const boost::filesystem::path& directory) {
  return readFromFile(directory / internal::getNameOfIdFile());
}

Map::Id Map::Id::readFromFile(const boost::filesystem::path& file) {
  Id id;
  const auto stream = mt::fopen(file, "r");
  mt::fread(stream.get(), &id, sizeof id);
  return id;
}

void Map::Id::writeToFile(const boost::filesystem::path& file) const {
  const auto stream = mt::fopen(file, "w");
  mt::fwrite(stream.get(), this, sizeof *this);
}

std::size_t Map::Limits::getMaxKeySize() {
  return internal::Shard::Limits::getMaxKeySize();
}

std::size_t Map::Limits::getMaxValueSize() {
  return internal::Shard::Limits::getMaxValueSize();
}

Map::Map(const boost::filesystem::path& directory, const Options& options)
    : lock_(directory, internal::getNameOfLockFile()) {
  checkOptions(options);
  const auto id_file = directory / internal::getNameOfIdFile();
  if (boost::filesystem::is_regular_file(id_file)) {
    mt::Check::isFalse(options.error_if_exists,
                       "A Multimap in '%s' does already exist",
                       boost::filesystem::canonical(directory).c_str());

    const auto id = Id::readFromFile(id_file);
    internal::checkVersion(id.major_version, id.minor_version);
    shards_.resize(id.num_shards);

  } else {
    mt::Check::isTrue(options.create_if_missing, "No Multimap found in '%s'",
                      boost::filesystem::canonical(directory).c_str());
    shards_.resize(mt::nextPrime(options.num_shards));
  }

  internal::Shard::Options shard_options;
  shard_options.block_size = options.block_size;
  shard_options.buffer_size = options.buffer_size;
  shard_options.readonly = options.readonly;
  shard_options.quiet = options.quiet;

  for (std::size_t i = 0; i != shards_.size(); ++i) {
    const auto prefix = directory / internal::getShardPrefix(i);
    shards_[i].reset(new internal::Shard(prefix, shard_options));
  }
}

Map::~Map() {
  if (!shards_.empty()) {
    Id id;
    id.block_size = shards_.front()->getBlockSize();
    id.num_shards = shards_.size();
    id.writeToFile(lock_.directory() / internal::getNameOfIdFile());
  }
}

void Map::put(const Bytes& key, const Bytes& value) {
  getShard(shards_, key).put(key, value);
}

Map::ListIterator Map::get(const Bytes& key) const {
  return getShard(shards_, key).get(key);
}

Map::MutableListIterator Map::getMutable(const Bytes& key) {
  return getShard(shards_, key).getMutable(key);
}

std::size_t Map::remove(const Bytes& key) {
  return getShard(shards_, key).remove(key);
}

std::size_t Map::removeAll(const Bytes& key, Callables::Predicate predicate) {
  return getShard(shards_, key).removeAll(key, predicate);
}

std::size_t Map::removeAllEqual(const Bytes& key, const Bytes& value) {
  return getShard(shards_, key).removeAllEqual(key, value);
}

bool Map::removeFirst(const Bytes& key, Callables::Predicate predicate) {
  return getShard(shards_, key).removeFirst(key, predicate);
}

bool Map::removeFirstEqual(const Bytes& key, const Bytes& value) {
  return getShard(shards_, key).removeFirstEqual(key, value);
}

std::size_t Map::replaceAll(const Bytes& key, Callables::Function function) {
  return getShard(shards_, key).replaceAll(key, function);
}

std::size_t Map::replaceAllEqual(const Bytes& key, const Bytes& old_value,
                                 const Bytes& new_value) {
  return getShard(shards_, key).replaceAllEqual(key, old_value, new_value);
}

bool Map::replaceFirst(const Bytes& key, Callables::Function function) {
  return getShard(shards_, key).replaceFirst(key, function);
}

bool Map::replaceFirstEqual(const Bytes& key, const Bytes& old_value,
                            const Bytes& new_value) {
  return getShard(shards_, key).replaceFirstEqual(key, old_value, new_value);
}

void Map::forEachKey(Callables::Procedure action) const {
  for (const auto& shard : shards_) {
    shard->forEachKey(action);
  }
}

void Map::forEachValue(const Bytes& key, Callables::Procedure action) const {
  getShard(shards_, key).forEachValue(key, action);
}

void Map::forEachValue(const Bytes& key, Callables::Predicate action) const {
  getShard(shards_, key).forEachValue(key, action);
}

void Map::forEachEntry(Callables::BinaryProcedure action) const {
  for (const auto& shard : shards_) {
    shard->forEachEntry(action);
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

bool Map::isReadOnly() const {
  MT_REQUIRE_FALSE(shards_.empty());
  return shards_.front()->isReadOnly();
}

namespace internal {

const std::string getFilePrefix() { return "multimap.map"; }

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
