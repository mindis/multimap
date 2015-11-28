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

#ifndef MULTIMAP_OPERATIONS_HPP_INCLUDED
#define MULTIMAP_OPERATIONS_HPP_INCLUDED

#include <vector>
#include <boost/filesystem/path.hpp>
#include "multimap/internal/Shard.hpp"
#include "multimap/Options.hpp"

namespace multimap {

std::vector<internal::Shard::Stats> stats(
    const boost::filesystem::path& directory);

void importFromBase64(const boost::filesystem::path& directory,
                      const boost::filesystem::path& input);
// Imports key-value pairs from a Base64-encoded text file denoted by file
// into the map located in the directory denoted by directory.
// Preconditions:
//   * The content in file follows the format described in Import / Export.
// Throws std::exception if:
//   * directory does not exist.
//   * directory does not contain a map.
//   * the map in directory is locked.
//   * file is not a regular file.

void importFromBase64(const boost::filesystem::path& directory,
                      const boost::filesystem::path& input,
                      const Options& options);
// Same as Import(directory, file) but creates a new map with default block
// size if the directory denoted by directory does not contain a map and
// create_if_missing is true.
// Preconditions:
//   * see Import(directory, file)
// Throws std::exception if:
//   * see Import(directory, file)
//   * block_size is not a power of two.

void exportToBase64(const boost::filesystem::path& directory,
                    const boost::filesystem::path& output);
// Exports all key-value pairs from the map located in the directory denoted
// by `directory` to a Base64-encoded text file denoted by `file`. If the file
// already exists, its content will be overridden.
//
// Tip: If `directory` and `file` point to different devices things will go
// faster. If `directory` points to an SSD things will go even faster, at
// least factor 2.
//
// Postconditions:
//   * The content in `file` follows the format described in Import / Export.
// Throws `std::exception` if:
//   * `directory` does not exist.
//   * `directory` does not contain a map.
//   * the map in directory is locked.
//   * `file` cannot be created.

void exportToBase64(const boost::filesystem::path& directory,
                    const boost::filesystem::path& output,
                    const Options& options);
// TODO Document this.

void optimize(const boost::filesystem::path& directory,
              const boost::filesystem::path& output, const Options& options);
// Optimizes the map located in the directory denoted by `directory`
// performing the following tasks:
//
//   * Defragmentation. All blocks which belong to the same list are written
//     sequentially to disk which improves locality and leads to better read
//     performance.
//   * Garbage collection. Values marked as deleted will be removed physically
//     which reduces the size of the map and also improves locality.
//
// Throws `std::exception` if:
//
//   * `directory` is not a directory.
//   * `directory` does not contain a map.
//   * the map in `directory` is locked.
//   * `directory` is not writable.
//   * `new_block_size` is not a power of two.

} // namespace multimap

#endif // MULTIMAP_OPERATIONS_HPP_INCLUDED
