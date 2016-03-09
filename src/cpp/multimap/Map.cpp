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
#include "multimap/internal/Base64.hpp"
#include "multimap/internal/TsvFileReader.hpp"

namespace multimap {

namespace {

void checkOptions(const Map::Options& options) {
  mt::Check::notZero(options.block_size, "Map's block size must be positive");
  mt::Check::isTrue(mt::isPowerOfTwo(options.block_size),
                    "Map's block size must be a power of two");
}

std::string getPrefix() { return "multimap.map"; }

std::string getNameOfIdFile() { return getPrefix() + ".id"; }

std::string getNameOfLockFile() { return getPrefix() + ".lock"; }

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

template <typename Procedure>
// Required interface:
// void process(const std::string& partition_prefix,
//              const internal::Partition::Options& partition_options,
//              size_t partition_index, size_t num_partitions);
void forEachPartition(const boost::filesystem::path& directory,
                      Procedure process) {
  mt::DirectoryLockGuard lock(directory.string(), getNameOfLockFile());
  const auto id = Map::Id::readFromDirectory(directory);
  Version::checkCompatibility(id.major_version, id.minor_version);

  internal::Partition::Options partition_options;
  partition_options.block_size = id.block_size;
  partition_options.readonly = true;

  for (size_t i = 0; i != id.num_partitions; ++i) {
    const auto partition_prefix = (directory / getPartitionPrefix(i)).string();
    process(partition_prefix, partition_options, i, id.num_partitions);
  }
}

}  // namespace

Map::Id Map::Id::readFromDirectory(const boost::filesystem::path& directory) {
  return readFromFile(directory / getNameOfIdFile());
}

Map::Id Map::Id::readFromFile(const boost::filesystem::path& filename) {
  Id id;
  const auto stream = mt::open(filename.string(), "r");
  mt::read(stream.get(), &id, sizeof id);
  return id;
}

void Map::Id::writeToFile(const boost::filesystem::path& filename) const {
  const auto stream = mt::open(filename.string(), "w");
  mt::write(stream.get(), this, sizeof *this);
}

uint32_t Map::Limits::maxKeySize() {
  return internal::Partition::Limits::maxKeySize();
}

uint32_t Map::Limits::maxValueSize() {
  return internal::Partition::Limits::maxValueSize();
}

Map::Map(const boost::filesystem::path& directory)
    : Map(directory, Options()) {}

Map::Map(const boost::filesystem::path& directory, const Options& options)
    : dlock_(directory.string(), getNameOfLockFile()) {
  checkOptions(options);
  internal::Partition::Options part_options;
  part_options.readonly = options.readonly;
  part_options.block_size = options.block_size;
  part_options.buffer_size = options.buffer_size;
  const auto id_filename = directory / getNameOfIdFile();
  if (boost::filesystem::is_regular_file(id_filename)) {
    mt::Check::isFalse(options.error_if_exists, "Map in '%s' already exists",
                       boost::filesystem::absolute(directory).c_str());
    const auto id = Id::readFromFile(id_filename);
    Version::checkCompatibility(id.major_version, id.minor_version);
    partitions_.resize(id.num_partitions);

  } else {
    mt::Check::isTrue(options.create_if_missing, "Map in '%s' does not exist",
                      boost::filesystem::absolute(directory).c_str());
    partitions_.resize(mt::nextPrime(options.num_partitions));
  }
  for (size_t i = 0; i != partitions_.size(); ++i) {
    const auto prefix = (directory / getPartitionPrefix(i)).string();
    partitions_[i].reset(new internal::Partition(prefix, part_options));
  }
}

Map::~Map() {
  if (!partitions_.empty()) {
    Id id;
    id.num_partitions = partitions_.size();
    id.block_size = partitions_.front()->getBlockSize();
    id.writeToFile(dlock_.directory() / getNameOfIdFile());
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
  mt::DirectoryLockGuard lock(directory.string(), getNameOfLockFile());
  const auto id = Id::readFromDirectory(directory);
  Version::checkCompatibility(id.major_version, id.minor_version);
  std::vector<Stats> stats;
  for (size_t i = 0; i != id.num_partitions; ++i) {
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
    if (!options.quiet) {
      mt::log() << "Importing " << file.string() << std::endl;
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
  std::ofstream stream(target.string());
  mt::check(stream.is_open(), "Could not create '%s'", target.c_str());

  const auto process =
      [&](const std::string& partition_prefix,
          const internal::Partition::Options& partition_options,
          size_t partition_index, size_t num_partitions) {

        if (!options.quiet) {
          mt::log(std::cout) << "Exporting partition " << (partition_index + 1)
                             << " of " << num_partitions << std::endl;
        }

        std::string base64_key;
        std::string base64_value;
        if (options.compare) {
          std::vector<Bytes> values;
          internal::Partition::forEachEntry(
              partition_prefix, partition_options,
              [&](const Range& key, Iterator* iter) {
                values.clear();
                values.reserve(iter->available());
                while (iter->hasNext()) {
                  values.push_back(iter->next().makeCopy());
                }
                std::sort(values.begin(), values.end(), options.compare);

                internal::Base64::encode(key, &base64_key);
                stream << base64_key;
                for (const auto& value : values) {
                  internal::Base64::encode(value, &base64_value);
                  stream << ' ' << base64_value;
                }
                stream << '\n';
              });
        } else {
          internal::Partition::forEachEntry(
              partition_prefix, partition_options,
              [&](const Range& key, Iterator* iter) {
                internal::Base64::encode(key, &base64_key);
                stream << base64_key;
                while (iter->hasNext()) {
                  internal::Base64::encode(iter->next(), &base64_value);
                  stream << ' ' << base64_value;
                }
                stream << '\n';
              });
        }
      };
  forEachPartition(directory, process);
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
  const auto id = Id::readFromDirectory(directory);
  Options new_options = options;
  new_options.error_if_exists = true;
  new_options.create_if_missing = true;
  if (options.block_size == 0) {
    new_options.block_size = id.block_size;
  }
  if (options.num_partitions == 0) {
    new_options.num_partitions = id.num_partitions;
  }
  Map new_map(target, new_options);

  forEachPartition(
      directory, [&](const std::string& partition_prefix,
                     const internal::Partition::Options& partition_options,
                     size_t partition_index, size_t num_partitions) {
        if (!options.quiet) {
          mt::log(std::cout) << "Optimizing partition " << (partition_index + 1)
                             << " of " << num_partitions << std::endl;
        }
        if (options.compare) {
          std::vector<Bytes> values;
          internal::Partition::forEachEntry(
              partition_prefix, partition_options,
              [&](const Range& key, Iterator* iter) {
                values.clear();
                values.reserve(iter->available());
                while (iter->hasNext()) {
                  values.push_back(iter->next().makeCopy());
                }
                std::sort(values.begin(), values.end(), options.compare);
                for (const auto& value : values) {
                  new_map.put(key, value);
                }
              });
        } else {
          internal::Partition::forEachEntry(
              partition_prefix, partition_options,
              [&](const Range& key, Iterator* iter) {
                while (iter->hasNext()) {
                  new_map.put(key, iter->next());
                }
              });
        }
      });
}

}  // namespace multimap
