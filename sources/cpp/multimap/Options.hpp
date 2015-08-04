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

#ifndef MULTIMAP_OPTIONS_HPP
#define MULTIMAP_OPTIONS_HPP

#include <cstddef>

namespace multimap {

// Converts a number in mebibytes to bytes.
inline std::size_t MiB(std::size_t mebibytes) { return mebibytes << 20; }

// Converts a number in gibibytes to bytes.
inline std::size_t GiB(std::size_t gibibytes) { return gibibytes << 30; }

struct Options {
  // The size of a data block in bytes.
  std::size_t block_size = 512;

  // The size of the in-memory block pool in bytes.
  std::size_t block_pool_memory = GiB(1);

  bool create_if_missing = false;

  bool verbose = false;
};

}  // namespace multimap

#endif  // MULTIMAP_OPTIONS_HPP
