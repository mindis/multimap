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

#include "multimap/Map.hpp"

#include <algorithm>
#include <iostream>
#include <boost/filesystem/operations.hpp>
#include "multimap/internal/Descriptor.hpp"
#include "multimap/internal/TsvFileWriter.hpp"
#include "multimap/internal/TsvFileReader.hpp"

namespace multimap {

namespace {

void checkOptions(const Map::Options& options) {
  mt::Check::notZero(options.block_size, "Map's block size must be positive");
  mt::Check::isTrue(mt::isPowerOfTwo(options.block_size),
                    "Map's block size must be a power of two");
}

const std::string& getPrefix() {
  static const auto prefix = internal::getCommonFilePrefix() + ".map";
  return prefix;
}

std::string getPartitionPrefix(size_t index) {
  return getPrefix() + '.' + std::to_string(index);
}

std::string getNameOfKeysFile(size_t index) {
  return internal::Partition::getNameOfMapFile(getPartitionPrefix(index));
}

std::string getNameOfStatsFile(size_t index) {
  return internal::Partition::getNameOfStatsFile(getPartitionPrefix(index));
}

std::string getNameOfValuesFile(size_t index) {
  return internal::Partition::getNameOfStoreFile(getPartitionPrefix(index));
}

}  // namespace

uint32_t Map::Limits::maxKeySize() {
  return internal::Partition::Limits::maxKeySize();
}

uint32_t Map::Limits::maxValueSize() {
  return internal::Partition::Limits::maxValueSize();
}

Map::Map(const boost::filesystem::path& directory)
    : Map(directory, Options()) {}

Map::Map(const boost::filesystem::path& directory, const Options& options)
    : dlock_(directory, internal::getNameOfLockFile()) {
  checkOptions(options);
  internal::Partition::Options part_options;
  part_options.readonly = options.readonly;
  part_options.block_size = options.block_size;
  const auto desc_filename = directory / internal::getNameOfDescriptorFile();
  if (boost::filesystem::is_regular_file(desc_filename)) {
    mt::Check::isFalse(options.error_if_exists, "Map in '%s' already exists",
                       boost::filesystem::absolute(directory).c_str());
    const auto desc = internal::Descriptor::readFromFile(
        desc_filename, internal::Descriptor::MAP);
    partitions_.resize(desc.num_partitions);

  } else {
    mt::Check::isTrue(options.create_if_missing, "Map in '%s' does not exist",
                      boost::filesystem::absolute(directory).c_str());
    partitions_.resize(mt::nextPrime(options.num_partitions));
    internal::Descriptor desc;
    desc.type = internal::Descriptor::MAP;
    desc.num_partitions = partitions_.size();
    desc.writeToFile(desc_filename);
  }
  for (size_t i = 0; i != partitions_.size(); i++) {
    const auto prefix = (directory / getPartitionPrefix(i)).string();
    partitions_[i].reset(new internal::Partition(prefix, part_options));
  }
}

std::vector<Map::Stats> Map::getStats() const {
  std::vector<Stats> stats;
  for (const auto& partition : partitions_) {
    stats.push_back(partition->getStats());
  }
  return stats;
}

Map::Stats Map::getTotalStats() const { return Stats::total(getStats()); }

bool Map::isReadOnly() const { return partitions_.front()->isReadOnly(); }

std::vector<Map::Stats> Map::stats(const boost::filesystem::path& directory) {
  mt::DirectoryLockGuard lock(directory, internal::getNameOfLockFile());
  const auto desc = internal::Descriptor::readFromDirectory(
      directory, internal::Descriptor::MAP);
  std::vector<Stats> stats;
  for (size_t i = 0; i != desc.num_partitions; i++) {
    const auto stats_file = directory / getNameOfStatsFile(i);
    stats.push_back(Stats::readFromFile(stats_file));
  }
  return stats;
}

void Map::importFromBase64(const boost::filesystem::path& directory,
                           const boost::filesystem::path& source) {
  Map::importFromBase64(directory, source, Options());
}

void Map::importFromBase64(const boost::filesystem::path& directory,
                           const boost::filesystem::path& source,
                           const Options& options) {
  Map map(directory, options);

  std::vector<boost::filesystem::path> files;
  if (boost::filesystem::is_regular_file(source)) {
    files.push_back(source);
  } else if (boost::filesystem::is_directory(source)) {
    files = mt::Files::list(source);
  } else {
    mt::fail("Not a regular file or directory '%s'", source.c_str());
  }

  Bytes key;
  Bytes value;
  for (const auto& file : files) {
    if (options.verbose) {
      mt::log() << "Importing " << file << std::endl;
    }
    internal::TsvFileReader reader(file);
    while (reader.read(&key, &value)) {
      map.put(key, value);
    }
  }
}

void Map::exportToBase64(const boost::filesystem::path& directory,
                         const boost::filesystem::path& target) {
  Map::exportToBase64(directory, target, Options());
}

void Map::exportToBase64(const boost::filesystem::path& directory,
                         const boost::filesystem::path& target,
                         const Options& options) {
  mt::DirectoryLockGuard lock(directory, internal::getNameOfLockFile());
  const auto desc = internal::Descriptor::readFromDirectory(
      directory, internal::Descriptor::MAP);
  internal::TsvFileWriter writer(target);

  for (size_t i = 0; i != desc.num_partitions; i++) {
    const auto prefix = directory / getPartitionPrefix(i);
    if (options.verbose) {
      mt::log() << "Exporting partition " << (i + 1) << " of "
                << desc.num_partitions << std::endl;
    }
    if (options.compare) {
      internal::Arena arena;
      std::vector<Range> values;
      internal::Partition::forEachEntry(
          prefix.string(), [&writer, &arena, &options, &values](
                               const Range& key, Iterator* iter) {
            arena.deallocateAll();
            values.reserve(iter->available());
            values.clear();
            while (iter->hasNext()) {
              values.push_back(iter->next().makeCopy(
                  [&arena](size_t size) { return arena.allocate(size); }));
            }
            std::sort(values.begin(), values.end(), options.compare);
            auto range_iter = makeRangeIterator(values.begin(), values.end());
            writer.write(key, &range_iter);
          });
    } else {
      internal::Partition::forEachEntry(
          prefix.string(), [&writer](const Range& key, Iterator* iter) {
            writer.write(key, iter);
          });
    }
  }
}

void Map::optimize(const boost::filesystem::path& directory,
                   const boost::filesystem::path& target) {
  Options options;
  options.keepBlockSize();
  options.keepNumPartitions();
  optimize(directory, target, options);
}

void Map::optimize(const boost::filesystem::path& directory,
                   const boost::filesystem::path& target,
                   const Options& options) {
  const auto desc = internal::Descriptor::readFromDirectory(
      directory, internal::Descriptor::MAP);
  const auto stats = Stats::readFromFile(directory / getNameOfStatsFile(0));
  Options new_options = options;
  new_options.error_if_exists = true;
  new_options.create_if_missing = true;
  if (options.block_size == 0) {
    new_options.block_size = stats.block_size;
  }
  if (options.num_partitions == 0) {
    new_options.num_partitions = desc.num_partitions;
  }
  Map new_map(target, new_options);

  for (size_t i = 0; i != desc.num_partitions; i++) {
    const auto prefix = directory / getPartitionPrefix(i);
    if (options.verbose) {
      mt::log() << "Optimizing partition " << (i + 1) << " of "
                << desc.num_partitions << std::endl;
    }
    if (options.compare) {
      internal::Arena arena;
      std::vector<Range> values;
      internal::Partition::forEachEntry(
          prefix.string(), [&new_map, &arena, &options, &values](
                               const Range& key, Iterator* iter) {
            arena.deallocateAll();
            values.reserve(iter->available());
            values.clear();
            while (iter->hasNext()) {
              values.push_back(iter->next().makeCopy(
                  [&arena](size_t size) { return arena.allocate(size); }));
            }
            std::sort(values.begin(), values.end(), options.compare);
            for (const auto& value : values) {
              new_map.put(key, value);
            }
          });
    } else {
      internal::Partition::forEachEntry(
          prefix.string(), [&new_map](const Range& key, Iterator* iter) {
            while (iter->hasNext()) {
              new_map.put(key, iter->next());
            }
          });
    }
  }
}

}  // namespace multimap
