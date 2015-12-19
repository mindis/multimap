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
//              std::size_t index, std::size_t nshards);
void forEachShard(const boost::filesystem::path& directory, Procdure process) {
  mt::DirectoryLockGuard lock(directory, internal::getNameOfLockFile());
  const auto id = Map::Id::readFromDirectory(directory);
  internal::checkVersion(id.major_version, id.minor_version);
  for (std::size_t i = 0; i != id.num_shards; ++i) {
    const auto prefix = directory / internal::getShardPrefix(i);
    process(prefix, i, id.num_shards);
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

std::size_t Map::Limits::maxKeySize() {
  return internal::Shard::Limits::maxKeySize();
}

std::size_t Map::Limits::maxValueSize() {
  return internal::Shard::Limits::maxValueSize();
}

Map::Map(const boost::filesystem::path& directory)
    : Map(directory, Options()) {}

Map::Map(const boost::filesystem::path& directory, const Options& options)
    : lock_(directory, internal::getNameOfLockFile()) {
  checkOptions(options);
  internal::Shard::Options shard_options;
  const auto id_file = directory / internal::getNameOfIdFile();
  if (boost::filesystem::is_regular_file(id_file)) {
    mt::Check::isFalse(options.error_if_exists, "Map in '%s' already exists",
                       boost::filesystem::absolute(directory).c_str());

    const auto id = Id::readFromFile(id_file);
    internal::checkVersion(id.major_version, id.minor_version);
    shards_.resize(id.num_shards);

  } else {
    mt::Check::isTrue(options.create_if_missing, "Map in '%s' does not exist",
                      boost::filesystem::absolute(directory).c_str());

    shard_options.block_size = options.block_size;
    shard_options.create_if_missing = true;
    shards_.resize(mt::nextPrime(options.num_shards));
  }

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

std::vector<internal::Shard::Stats> Map::stats(
    const boost::filesystem::path& directory) {
  mt::DirectoryLockGuard lock(directory, internal::getNameOfLockFile());
  const auto id = Id::readFromDirectory(directory);
  internal::checkVersion(id.major_version, id.minor_version);
  std::vector<internal::Shard::Stats> stats;
  for (std::size_t i = 0; i != id.num_shards; ++i) {
    const auto stats_file = directory / internal::getNameOfStatsFile(i);
    stats.push_back(internal::Shard::Stats::readFromFile(stats_file));
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
    std::ifstream stream(file.string());
    mt::check(stream, "Could not open '%s'", file.c_str());

    if (!options.quiet) {
      mt::log(std::cout) << "Importing " << file << std::endl;
    }

    std::string base64_key;
    std::string base64_value;
    std::string binary_key;
    std::string binary_value;
    if (stream >> base64_key) {
      internal::Base64::decode(base64_key, &binary_key);
      while (stream) {
        switch (stream.peek()) {
          case '\n':
          case '\r':
            stream >> base64_key;
            internal::Base64::decode(base64_key, &binary_key);
            break;
          case '\f':
          case '\t':
          case '\v':
          case ' ':
            stream.ignore();
            break;
          default:
            stream >> base64_value;
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
  mt::check(stream, "Could not create '%s'", output.c_str());

  const auto print_status = [&options](std::size_t index, std::size_t nshards) {
    if (!options.quiet) {
      mt::log(std::cout) << "Exporting shard " << (index + 1) << " of "
                         << nshards << std::endl;
    }
  };

  std::string base64_key;
  std::string base64_value;
  if (options.compare) {
    std::vector<std::string> sorted_values;
    forEachShard(directory, [&](const boost::filesystem::path& prefix,
                                std::size_t index, std::size_t nshards) {
      print_status(index, nshards);
      internal::Shard::forEachEntry(prefix, [&](const Bytes& key,
                                                Map::Iterator&& iter) {
        sorted_values.clear();
        sorted_values.reserve(iter.available());
        while (iter.hasNext()) {
          sorted_values.push_back(iter.next().toString());
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
    forEachShard(directory, [&](const boost::filesystem::path& prefix,
                                std::size_t index, std::size_t nshards) {
      print_status(index, nshards);
      internal::Shard::forEachEntry(
          prefix, [&](const Bytes& key, Map::Iterator&& iter) {
            internal::Base64::encode(key, &base64_key);
            stream << base64_key;
            while (iter.hasNext()) {
              internal::Base64::encode(iter.next(), &base64_value);
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
  options.keep_block_size();
  options.keep_num_shards();
  optimize(directory, output, options);
}

void Map::optimize(const boost::filesystem::path& directory,
                   const boost::filesystem::path& output,
                   const Options& options) {
  std::unique_ptr<Map> new_map;
  forEachShard(directory, [&](const boost::filesystem::path& prefix,
                              std::size_t index, std::size_t nshards) {
    if (index == 0) {
      const auto old_id = Id::readFromDirectory(directory);
      Options new_map_options = options;
      new_map_options.error_if_exists = true;
      new_map_options.create_if_missing = true;
      if (options.block_size == 0) {
        new_map_options.block_size = old_id.block_size;
      }
      if (options.num_shards == 0) {
        new_map_options.num_shards = old_id.num_shards;
      }
      new_map.reset(new Map(output, new_map_options));
    }

    if (!options.quiet) {
      mt::log(std::cout) << "Optimizing shard " << (index + 1) << " of "
                         << nshards << std::endl;
    }

    if (options.compare) {
      std::vector<std::string> sorted_values;
      internal::Shard::forEachEntry(prefix, [&](const Bytes& key,
                                                Map::Iterator&& iter) {
        sorted_values.clear();
        sorted_values.reserve(iter.available());
        while (iter.hasNext()) {
          sorted_values.push_back(iter.next().toString());
        }
        std::sort(sorted_values.begin(), sorted_values.end(), options.compare);
        for (const auto& value : sorted_values) {
          new_map->put(key, value);
        }
      });
    } else {
      internal::Shard::forEachEntry(
          prefix, [&](const Bytes& key, Map::Iterator&& iter) {
            while (iter.hasNext()) {
              new_map->put(key, iter.next());
            }
          });
    }
  });
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
  mt::check(major_version == MAJOR_VERSION,
            "Version check failed. The Multimap you are trying to open "
            "was created with version %u.%u of the library. Your "
            "installed version is %u.%u which is not compatible.",
            major_version, minor_version, MAJOR_VERSION, MINOR_VERSION);
}

}  // namespace internal
}  // namespace multimap
