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

#ifndef MULTIMAP_OPTIONS_HPP_INCLUDED
#define MULTIMAP_OPTIONS_HPP_INCLUDED

#include <cstdint>
#include <functional>
#include "multimap/Bytes.hpp"

namespace multimap {

struct Options {

  uint32_t block_size = 512;

  uint32_t num_partitions = 23;

  uint32_t buffer_size = mt::MiB(1);

  bool create_if_missing = false;

  bool error_if_exists = false;

  bool readonly = false;

  bool quiet = false;
  // Prints out status messages for long running jobs from `operations.hpp`.

  std::function<bool(const Bytes&, const Bytes&)> compare;
  // Optional: Compare function which returns `true` if the left operand is
  // less than the right operand. Returns `false` otherwise.

  void keepNumPartitions() { num_partitions = 0; }
  // Indicates to some operations to leave the number of partitions unchanged.

  void keepBlockSize() { block_size = 0; }
  // Indicates to some operations to leave the block size unchanged.
};

}  // namespace multimap

#endif  // MULTIMAP_OPTIONS_HPP_INCLUDED
