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

#include "multimap/common.hpp"

namespace multimap {

// A pure data holder used to configure an instantiation of class Map.
struct Options {
  // Determines the block size of a newly created map. The value is ignored if
  // the map already exists when opened. The value must be a power of two. Have
  // a look at Choosing the block size for more information.
  std::size_t block_size = 512;

  // Determines whether a map has to be created if it does not exist.
  bool create_if_missing = false;

  // Determines whether an already existing map should be treated as an error.
  bool error_if_exists = false;

  // Determines if the map should be opened in write-only mode. This will
  // enable some optimizations for putting a large number of values, but will
  // disable the ability to retrieve values. Users normally should leave this
  // parameter alone.
  bool write_only_mode = false;
};

}  // namespace multimap

#endif  // MULTIMAP_OPTIONS_HPP
