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

// -----------------------------------------------------------------------------
// Documentation:  https://multimap.io/cppreference/#optionshpp
// -----------------------------------------------------------------------------

#ifndef MULTIMAP_OPTIONS_HPP_INCLUDED
#define MULTIMAP_OPTIONS_HPP_INCLUDED

#include <functional>
#include "multimap/callables.hpp"

namespace multimap {

struct Options {
  size_t block_size = 512;
  size_t num_partitions = 23;

  bool create_if_missing = false;
  bool error_if_exists = false;
  bool readonly = false;
  bool verbose = true;

  Compare compare;

  void keepNumPartitions() { num_partitions = 0; }
  void keepBlockSize() { block_size = 0; }

  Options() = default;
};

}  // namespace multimap

#endif  // MULTIMAP_OPTIONS_HPP_INCLUDED
