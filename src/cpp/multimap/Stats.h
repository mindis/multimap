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
// Documentation:  https://multimap.io/cppreference/#statshpp
// -----------------------------------------------------------------------------

#ifndef MULTIMAP_STATS_H_
#define MULTIMAP_STATS_H_

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#include <boost/filesystem/path.hpp>  // NOLINT
#include "multimap/thirdparty/mt/assert.h"

namespace multimap {

struct Stats {
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

  static Stats readFromFile(const boost::filesystem::path& file_path);

  void writeToFile(const boost::filesystem::path& file_path) const;

  std::vector<uint64_t> toVector() const;

  Stats() = default;
};

MT_STATIC_ASSERT_SIZEOF(Stats, 104, 104);

}  // namespace multimap

#endif  // MULTIMAP_STATS_H_
