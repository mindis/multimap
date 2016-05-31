// This file is part of Multimap.  http://multimap.io
//
// Copyright (C) 2015-2016  Martin Trenkmann
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

#include "multimap/Map.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include <boost/filesystem/operations.hpp>  // NOLINT
#include "multimap/internal/TsvFileReader.h"
#include "multimap/internal/TsvFileWriter.h"
#include "multimap/thirdparty/mt/check.h"

namespace fs = boost::filesystem;

namespace multimap {

namespace {

void checkOptions(const Options& options) {
  mt::Check::notZero(options.block_size, "Map's block size must be positive");
  mt::Check::isTrue(mt::isPowerOfTwo(options.block_size),
                    "Map's block size must be a power of two");
}

void checkDescriptor(const internal::Descriptor& descriptor,
                     const fs::path& directory) {
  mt::Check::isEqual(
      internal::Descriptor::TYPE_MAP, descriptor.map_type,
      "Wrong map type: expected type %s but found type %s in %s",
      internal::Descriptor::toString(internal::Descriptor::TYPE_MAP),
      internal::Descriptor::toString(descriptor.map_type), directory.c_str());
}

std::string getFilePrefix() {
  return internal::Descriptor::getFilePrefix() + ".map";
}

std::string getPartitionPrefix(size_t index) {
  return getFilePrefix() + '.' + std::to_string(index);
}

}  // namespace

size_t Map::Limits::maxKeySize() {
  return internal::Partition::Limits::maxKeySize();
}

size_t Map::Limits::maxValueSize() {
  return internal::Partition::Limits::maxValueSize();
}

Map::Map(const fs::path& directory) : Map(directory, Options()) {}

Map::Map(const fs::path& directory, const Options& options)
    : dlock_(directory.string()) {
  checkOptions(options);
  Options partition_options;
  partition_options.readonly = options.readonly;
  partition_options.block_size = options.block_size;
  internal::Descriptor descriptor;
  if (internal::Descriptor::tryReadFromDirectory(directory, &descriptor)) {
    checkDescriptor(descriptor, directory);
    mt::Check::isFalse(options.error_if_exists, "Map in %s already exists",
                       fs::absolute(directory).c_str());
    partitions_.resize(descriptor.num_partitions);

  } else {
    mt::Check::isTrue(options.create_if_missing, "Map in %s does not exist",
                      fs::absolute(directory).c_str());
    partitions_.resize(mt::nextPrime(options.num_partitions));
    descriptor.map_type = internal::Descriptor::TYPE_MAP;
    descriptor.num_partitions = partitions_.size();
    descriptor.writeToDirectory(directory);
  }
  for (size_t i = 0; i != partitions_.size(); i++) {
    const fs::path prefix = directory / getPartitionPrefix(i);
    partitions_[i].reset(new internal::Partition(prefix, partition_options));
  }
}

void Map::put(const Slice& key, const Slice& value) {
  getPartition(key)->put(key, value);
}

std::unique_ptr<Iterator> Map::get(const Slice& key) const {
  return getPartition(key)->get(key);
}

size_t Map::remove(const Slice& key) { return getPartition(key)->remove(key); }

bool Map::removeFirstEqual(const Slice& key, const Slice& value) {
  return getPartition(key)->removeFirstEqual(key, value);
}

size_t Map::removeAllEqual(const Slice& key, const Slice& value) {
  return getPartition(key)->removeAllEqual(key, value);
}

bool Map::removeFirstMatch(const Slice& key, Predicate predicate) {
  return getPartition(key)->removeFirstMatch(key, predicate);
}

size_t Map::removeFirstMatch(Predicate predicate) {
  size_t num_values_removed = 0;
  for (const auto& partition : partitions_) {
    num_values_removed = partition->removeFirstMatch(predicate);
    if (num_values_removed != 0) break;
  }
  return num_values_removed;
}

size_t Map::removeAllMatches(const Slice& key, Predicate predicate) {
  return getPartition(key)->removeAllMatches(key, predicate);
}

std::pair<size_t, size_t> Map::removeAllMatches(Predicate predicate) {
  size_t num_keys_removed = 0;
  size_t num_values_removed = 0;
  for (const auto& partition : partitions_) {
    const auto result = partition->removeAllMatches(predicate);
    num_keys_removed += result.first;
    num_values_removed += result.second;
  }
  return std::make_pair(num_keys_removed, num_values_removed);
}

bool Map::replaceFirstEqual(const Slice& key, const Slice& old_value,
                            const Slice& new_value) {
  return getPartition(key)->replaceFirstEqual(key, old_value, new_value);
}

size_t Map::replaceAllEqual(const Slice& key, const Slice& old_value,
                            const Slice& new_value) {
  return getPartition(key)->replaceAllEqual(key, old_value, new_value);
}

bool Map::replaceFirstMatch(const Slice& key, Function map) {
  return getPartition(key)->replaceFirstMatch(key, map);
}

size_t Map::replaceAllMatches(const Slice& key, Function map) {
  return getPartition(key)->replaceAllMatches(key, map);
}

void Map::forEachKey(Procedure process) const {
  for (const auto& partition : partitions_) {
    partition->forEachKey(process);
  }
}

void Map::forEachValue(const Slice& key, Procedure process) const {
  getPartition(key)->forEachValue(key, process);
}

void Map::forEachEntry(BinaryProcedure process) const {
  for (const auto& partition : partitions_) {
    partition->forEachEntry(process);
  }
}

std::vector<Stats> Map::getStats() const {
  std::vector<Stats> stats;
  for (const auto& partition : partitions_) {
    stats.push_back(partition->getStats());
  }
  return stats;
}

Stats Map::getTotalStats() const { return Stats::total(getStats()); }

std::vector<Stats> Map::stats(const fs::path& directory) {
  internal::DirectoryLock lock(directory.string());
  const auto descriptor = internal::Descriptor::readFromDirectory(directory);
  checkDescriptor(descriptor, directory);
  std::vector<Stats> stats;
  for (int i = 0; i < descriptor.num_partitions; i++) {
    const fs::path prefix = directory / getPartitionPrefix(i);
    stats.push_back(internal::Partition::stats(prefix));
  }
  return stats;
}

void Map::importFromBase64(const fs::path& directory, const fs::path& source) {
  Map::importFromBase64(directory, source, Options());
}

void Map::importFromBase64(const fs::path& directory, const fs::path& source,
                           const Options& options) {
  Map map(directory, options);

  std::vector<fs::path> file_paths;
  if (fs::is_regular_file(source)) {
    file_paths.push_back(source);
  } else if (fs::is_directory(source)) {
    file_paths = mt::listFiles(source);
  } else {
    mt::fail("No such file or directory: %s", source.c_str());
  }

  Bytes key;
  Bytes value;
  for (const auto& file_path : file_paths) {
    if (options.verbose) {
      mt::log() << "Importing " << file_path << std::endl;
    }
    internal::TsvFileReader reader(file_path);
    while (reader.read(&key, &value)) {
      map.put(key, value);
    }
  }
}

void Map::exportToBase64(const fs::path& directory, const fs::path& target) {
  Map::exportToBase64(directory, target, Options());
}

void Map::exportToBase64(const fs::path& directory, const fs::path& target,
                         const Options& options) {
  internal::DirectoryLock lock(directory.string());
  const auto descriptor = internal::Descriptor::readFromDirectory(directory);
  checkDescriptor(descriptor, directory);
  internal::TsvFileWriter writer(target);

  for (int i = 0; i < descriptor.num_partitions; i++) {
    const fs::path prefix = directory / getPartitionPrefix(i);
    if (options.verbose) {
      mt::log() << "Exporting partition " << (i + 1) << " of "
                << descriptor.num_partitions << std::endl;
    }
    if (options.compare) {
      Arena arena;
      std::vector<Slice> values;
      internal::Partition::forEachEntry(
          prefix, [&writer, &arena, &options, &values](const Slice& key,
                                                       Iterator* iter) {
            arena.deallocateAll();
            values.reserve(iter->available());
            values.clear();
            while (iter->hasNext()) {
              values.push_back(iter->next().makeCopy(&arena));
            }
            std::sort(values.begin(), values.end(), options.compare);
            auto range_iter = makeRangeIterator(values.begin(), values.end());
            writer.write(key, &range_iter);
          });
    } else {
      internal::Partition::forEachEntry(
          prefix, [&writer](const Slice& key, Iterator* iter) {
            writer.write(key, iter);
          });
    }
  }
}

void Map::optimize(const fs::path& directory, const fs::path& target) {
  Options options;
  options.keepBlockSize();
  options.keepNumPartitions();
  optimize(directory, target, options);
}

void Map::optimize(const fs::path& directory, const fs::path& target,
                   const Options& options) {
  const auto descriptor = internal::Descriptor::readFromDirectory(directory);
  checkDescriptor(descriptor, directory);
  fs::path prefix = directory / getPartitionPrefix(0);
  const Stats stats = internal::Partition::stats(prefix);
  Options new_options = options;
  new_options.error_if_exists = true;
  new_options.create_if_missing = true;
  if (options.block_size == 0) {
    new_options.block_size = stats.block_size;
  }
  if (options.num_partitions == 0) {
    new_options.num_partitions = descriptor.num_partitions;
  }
  Map new_map(target, new_options);

  for (int i = 0; i < descriptor.num_partitions; i++) {
    prefix = directory / getPartitionPrefix(i);
    if (options.verbose) {
      mt::log() << "Optimizing partition " << (i + 1) << " of "
                << descriptor.num_partitions << std::endl;
    }
    if (options.compare) {
      Arena arena;
      std::vector<Slice> values;
      internal::Partition::forEachEntry(
          prefix, [&new_map, &arena, &options, &values](const Slice& key,
                                                        Iterator* iter) {
            arena.deallocateAll();
            values.reserve(iter->available());
            values.clear();
            while (iter->hasNext()) {
              values.push_back(iter->next().makeCopy(&arena));
            }
            std::sort(values.begin(), values.end(), options.compare);
            for (const auto& value : values) {
              new_map.put(key, value);
            }
          });
    } else {
      internal::Partition::forEachEntry(
          prefix, [&new_map](const Slice& key, Iterator* iter) {
            while (iter->hasNext()) {
              new_map.put(key, iter->next());
            }
          });
    }
  }
}

internal::Partition* Map::getPartition(const Slice& key) {
  return partitions_[std::hash<Slice>()(key) % partitions_.size()].get();
}

const internal::Partition* Map::getPartition(const Slice& key) const {
  return partitions_[std::hash<Slice>()(key) % partitions_.size()].get();
}

}  // namespace multimap
