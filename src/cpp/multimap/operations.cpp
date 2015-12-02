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

#include "multimap/operations.hpp"

#include <iostream>
#include <boost/filesystem/operations.hpp>
#include "multimap/internal/Base64.hpp"
#include "multimap/Map.hpp"

namespace multimap {

void forEachShard(
    const boost::filesystem::path& directory,
    std::function<void(const boost::filesystem::path& /* prefix */,
                       std::size_t /* index */, std::size_t /* nshards */)>
        action) {
  mt::DirectoryLockGuard lock(directory, internal::getNameOfLockFile());
  const auto id = Map::Id::readFromDirectory(directory);
  internal::checkVersion(id.major_version, id.minor_version);
  for (std::size_t i = 0; i != id.num_shards; ++i) {
    const auto prefix = directory / internal::getShardPrefix(i);
    action(prefix, i, id.num_shards);
  }
}

std::vector<internal::Shard::Stats> stats(
    const boost::filesystem::path& directory) {
  mt::DirectoryLockGuard lock(directory, internal::getNameOfLockFile());
  const auto id = Map::Id::readFromDirectory(directory);
  internal::checkVersion(id.major_version, id.minor_version);
  std::vector<internal::Shard::Stats> stats;
  for (std::size_t i = 0; i != id.num_shards; ++i) {
    const auto stats_file = directory / internal::getNameOfStatsFile(i);
    stats.push_back(internal::Shard::Stats::readFromFile(stats_file));
  }
  return stats;
}

void importFromBase64(const boost::filesystem::path& directory,
                      const boost::filesystem::path& input) {
  Options options;
  options.error_if_exists = false;
  options.create_if_missing = false;
  importFromBase64(directory, input, options);
}

void importFromBase64(const boost::filesystem::path& directory,
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
      if (!is_hidden(it->path())) {
        import_file(it->path());
      }
    }
  }
}

void exportToBase64(const boost::filesystem::path& directory,
                    const boost::filesystem::path& output) {
  exportToBase64(directory, output, Options());
}

void exportToBase64(const boost::filesystem::path& directory,
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
                                                Map::ListIterator&& iter) {
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
          prefix, [&](const Bytes& key, Map::ListIterator&& iter) {
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

void optimize(const boost::filesystem::path& directory,
              const boost::filesystem::path& output, const Options& options) {
  std::unique_ptr<Map> new_map;
  forEachShard(directory, [&](const boost::filesystem::path& prefix,
                              std::size_t index, std::size_t nshards) {
    if (index == 0) {
      const auto old_id = Map::Id::readFromDirectory(directory);
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
                                                Map::ListIterator&& iter) {
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
          prefix, [&](const Bytes& key, Map::ListIterator&& iter) {
            while (iter.hasNext()) {
              new_map->put(key, iter.next());
            }
          });
    }
  });
}

} // namespace multimap
