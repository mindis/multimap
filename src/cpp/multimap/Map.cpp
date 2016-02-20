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

namespace multimap {

namespace {

void checkOptions(const Options& options) {
  mt::Check::notZero(options.block_size, "Map's block size must be positive");
  mt::Check::isTrue(mt::isPowerOfTwo(options.block_size),
                    "Map's block size must be a power of two");
}

template <typename Procdure>
// Required interface:
// void process(const boost::filesystem::path& prefix,
//              size_t index, size_t ntables);
void forEachTable(const boost::filesystem::path& directory, Procdure process) {
  mt::DirectoryLockGuard lock(directory, internal::getNameOfLockFile());
  const auto id = Map::Id::readFromDirectory(directory);
  internal::checkVersion(id.major_version, id.minor_version);
  for (size_t i = 0; i != id.num_partitions; ++i) {
    const auto prefix = directory / internal::getTablePrefix(i);
    process(prefix, i, id.num_partitions);
  }
}

}  // namespace

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

uint32_t Map::Limits::maxKeySize() {
  return internal::Partition::Limits::maxKeySize();
}

uint32_t Map::Limits::maxValueSize() {
  return internal::Partition::Limits::maxValueSize();
}

Map::Map(const boost::filesystem::path& directory)
    : Map(directory, Options()) {}

Map::Map(const boost::filesystem::path& directory, const Options& options)
    : lock_(directory, internal::getNameOfLockFile()) {
  checkOptions(options);
  internal::Partition::Options part_options;
  part_options.readonly = options.readonly;
  part_options.block_size = options.block_size;
  part_options.buffer_size = options.buffer_size;
  part_options.create_if_missing = options.create_if_missing;
  const auto id_filename = directory / internal::getNameOfIdFile();
  if (boost::filesystem::is_regular_file(id_filename)) {
    mt::Check::isFalse(options.error_if_exists, "Map in '%s' already exists",
                       boost::filesystem::absolute(directory).c_str());
    const auto id = Id::readFromFile(id_filename);
    internal::checkVersion(id.major_version, id.minor_version);
    partitions_.resize(id.num_partitions);

  } else {
    mt::Check::isTrue(options.create_if_missing, "Map in '%s' does not exist",
                      boost::filesystem::absolute(directory).c_str());
    partitions_.resize(mt::nextPrime(options.num_partitions));
  }
  for (size_t i = 0; i != partitions_.size(); ++i) {
    const auto prefix = directory / internal::getTablePrefix(i);
    partitions_[i].reset(new internal::Partition(prefix, part_options));
  }
}

Map::~Map() {
  if (!partitions_.empty()) {
    Id id;
    id.block_size = partitions_.front()->getBlockSize();
    id.num_partitions = partitions_.size();
    id.writeToFile(lock_.directory() / internal::getNameOfIdFile());
  }
}

std::vector<Map::Stats> Map::getStats() const {
  std::vector<Stats> stats;
  for (const auto& table : partitions_) {
    stats.push_back(table->getStats());
  }
  return stats;
}

Map::Stats Map::getTotalStats() const { return Stats::total(getStats()); }

bool Map::isReadOnly() const { return partitions_.front()->isReadOnly(); }

std::vector<Map::Stats> Map::stats(const boost::filesystem::path& directory) {
  mt::DirectoryLockGuard lock(directory, internal::getNameOfLockFile());
  const auto id = Id::readFromDirectory(directory);
  internal::checkVersion(id.major_version, id.minor_version);
  std::vector<Stats> stats;
  for (size_t i = 0; i != id.num_partitions; ++i) {
    const auto stats_file = directory / internal::getNameOfStatsFile(i);
    stats.push_back(Stats::readFromFile(stats_file));
  }
  return stats;
}

void Map::importFromBase64(const boost::filesystem::path& directory,
                           const boost::filesystem::path& input) {
  Map::importFromBase64(directory, input, Options());
}

void Map::importFromBase64(const boost::filesystem::path& directory,
                           const boost::filesystem::path& input,
                           const Options& options) {
  Map map(directory, options);

  const auto import_file = [&](const boost::filesystem::path& file) {
    std::ifstream input(file.string());
    mt::check(input.is_open(), "Could not open '%s'", file.c_str());

    if (!options.quiet) {
      mt::log(std::cout) << "Importing " << file << std::endl;
    }

    std::string base64_key;
    std::string base64_value;
    std::string binary_key;
    std::string binary_value;
    if (input >> base64_key) {
      internal::Base64::decode(base64_key, &binary_key);
      while (input) {
        switch (input.peek()) {
          case '\n':
          case '\r':
            input >> base64_key;
            internal::Base64::decode(base64_key, &binary_key);
            break;
          case '\f':
          case '\t':
          case '\v':
          case ' ':
            input.ignore();
            break;
          default:
            input >> base64_value;
            internal::Base64::decode(base64_value, &binary_value);
            map.put(binary_key, binary_value);
        }
      }
    }
  };

  const auto is_hidden = [](const boost::filesystem::path& path) {
    return path.filename().string().front() == '.';
  };

  if (boost::filesystem::is_regular_file(input)) {
    import_file(input);
  } else if (boost::filesystem::is_directory(input)) {
    boost::filesystem::directory_iterator end;
    for (boost::filesystem::directory_iterator it(input); it != end; ++it) {
      const auto path = it->path();
      if (boost::filesystem::is_regular_file(path) && !is_hidden(path)) {
        import_file(it->path());
      }
    }
  } else {
    mt::fail("No such file or directory '%s'", input.c_str());
  }
}

void Map::exportToBase64(const boost::filesystem::path& directory,
                         const boost::filesystem::path& output) {
  Map::exportToBase64(directory, output, Options());
}

void Map::exportToBase64(const boost::filesystem::path& directory,
                         const boost::filesystem::path& output,
                         const Options& options) {
  std::ofstream stream(output.string());
  mt::check(stream.is_open(), "Could not create '%s'", output.c_str());

  const auto print_status = [&options](size_t index, size_t npartitions) {
    if (!options.quiet) {
      mt::log(std::cout) << "Exporting partition " << (index + 1) << " of "
                         << npartitions << std::endl;
    }
  };

  std::string base64_key;
  std::string base64_value;
  if (options.compare) {
    std::vector<std::string> sorted_values;
    forEachTable(directory, [&](const boost::filesystem::path& prefix,
                                size_t index, size_t npartitions) {
      print_status(index, npartitions);
      internal::Partition::forEachEntry(prefix, [&](const Bytes& key,
                                                       Iterator* iter) {
        sorted_values.clear();
        sorted_values.reserve(iter->available());
        while (iter->hasNext()) {
          sorted_values.push_back(iter->next().toString());
        }
        std::sort(sorted_values.begin(), sorted_values.end(), options.compare);

        internal::Base64::encode(key, &base64_key);
        stream << base64_key;
        for (const auto& value : sorted_values) {
          internal::Base64::encode(value, &base64_value);
          stream << ' ' << base64_value;
        }
        stream << '\n';
      });
    });

  } else {
    forEachTable(directory, [&](const boost::filesystem::path& prefix,
                                size_t index, size_t npartitions) {
      print_status(index, npartitions);
      internal::Partition::forEachEntry(
          prefix, [&](const Bytes& key, Iterator* iter) {
            internal::Base64::encode(key, &base64_key);
            stream << base64_key;
            while (iter->hasNext()) {
              internal::Base64::encode(iter->next(), &base64_value);
              stream << ' ' << base64_value;
            }
            stream << '\n';
          });
    });
  }
}

void Map::optimize(const boost::filesystem::path& directory,
                   const boost::filesystem::path& output) {
  Options options;
  options.keepBlockSize();
  options.keepNumPartitions();
  optimize(directory, output, options);
}

void Map::optimize(const boost::filesystem::path& directory,
                   const boost::filesystem::path& output,
                   const Options& options) {
  std::unique_ptr<Map> new_map;
  forEachTable(directory, [&](const boost::filesystem::path& prefix,
                              size_t index, size_t ntables) {
    if (index == 0) {
      const auto old_id = Id::readFromDirectory(directory);
      Options new_map_options = options;
      new_map_options.error_if_exists = true;
      new_map_options.create_if_missing = true;
      if (options.block_size == 0) {
        new_map_options.block_size = old_id.block_size;
      }
      if (options.num_partitions == 0) {
        new_map_options.num_partitions = old_id.num_partitions;
      }
      new_map.reset(new Map(output, new_map_options));
    }

    if (!options.quiet) {
      mt::log(std::cout) << "Optimizing partition " << (index + 1) << " of "
                         << ntables << std::endl;
    }

    if (options.compare) {
      std::vector<std::string> sorted_values;
      internal::Partition::forEachEntry(prefix, [&](const Bytes& key,
                                                       Iterator* iter) {
        sorted_values.clear();
        sorted_values.reserve(iter->available());
        while (iter->hasNext()) {
          sorted_values.push_back(iter->next().toString());
        }
        std::sort(sorted_values.begin(), sorted_values.end(), options.compare);
        for (const auto& value : sorted_values) {
          new_map->put(key, value);
        }
      });
    } else {
      internal::Partition::forEachEntry(
          prefix, [&](const Bytes& key, Iterator* iter) {
            while (iter->hasNext()) {
              new_map->put(key, iter->next());
            }
          });
    }
  });
}

namespace internal {

const std::string getFilePrefix() { return "multimap.map"; }

const std::string getNameOfIdFile() { return getFilePrefix() + ".id"; }

const std::string getNameOfLockFile() { return getFilePrefix() + ".lock"; }

const std::string getTablePrefix(uint32_t index) {
  return getFilePrefix() + '.' + std::to_string(index);
}

const std::string getNameOfKeysFile(uint32_t index) {
  return Partition::getNameOfKeysFile(getTablePrefix(index));
}

const std::string getNameOfStatsFile(uint32_t index) {
  return Partition::getNameOfStatsFile(getTablePrefix(index));
}

const std::string getNameOfValuesFile(uint32_t index) {
  return Partition::getNameOfValuesFile(getTablePrefix(index));
}

void checkVersion(uint64_t major_version, uint64_t minor_version) {
  mt::check(major_version == Version::MAJOR,
            "Version check failed. The Multimap you are trying to open "
            "was created with version %u.%u of the library. Your "
            "installed version is %u.%u which is not compatible.",
            major_version, minor_version, Version::MAJOR, Version::MINOR);
}

}  // namespace internal
}  // namespace multimap
