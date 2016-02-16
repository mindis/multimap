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
// Documentation:  http://multimap.io/cppreference/#class-stats
// -----------------------------------------------------------------------------

#ifndef MULTIMAP_INTERNAL_STATS_HPP_INCLUDED
#define MULTIMAP_INTERNAL_STATS_HPP_INCLUDED

#include <cstdint>
#include <string>
#include <vector>
#include <boost/filesystem/path.hpp>
#include "multimap/thirdparty/mt/mt.hpp"

namespace multimap {
namespace internal {

struct Stats {
  // Needs to be synchronized with class Map#Stats in Java.
  uint64_t block_size = 0;
  uint64_t key_size_avg = 0;
  uint64_t key_size_max = 0;
  uint64_t key_size_min = 0;
  uint64_t list_size_avg = 0;
  uint64_t list_size_max = 0;
  uint64_t list_size_min = 0;
  uint64_t num_blocks = 0;
  uint64_t num_keys_total = 0;
  uint64_t num_keys_valid = 0;
  uint64_t num_values_total = 0;
  uint64_t num_values_valid = 0;
  uint64_t num_partitions = 0;

  static const std::vector<std::string>& names();

  static Stats total(const std::vector<Stats>& stats);

  static Stats max(const std::vector<Stats>& stats);

  static Stats readFromFile(const boost::filesystem::path& file);

  void writeToFile(const boost::filesystem::path& file) const;

  std::vector<uint64_t> toVector() const;
};

static_assert(std::is_standard_layout<Stats>::value,
              "Stats is no standard layout type");

static_assert(mt::hasExpectedSize<Stats>(104, 104),
              "Stats does not have expected size");
// sizeof(Stats) must be equal on 32- and 64-bit systems for portability.

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_STATS_HPP_INCLUDED
