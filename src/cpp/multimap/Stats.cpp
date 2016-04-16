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

#include "multimap/Stats.hpp"

#include <cmath>
#include "multimap/thirdparty/mt/assert.hpp"
#include "multimap/thirdparty/mt/fileio.hpp"

namespace multimap {

const std::vector<std::string>& Stats::names() {
  static std::vector<std::string> names = {
      "block_size",     "key_size_avg",     "key_size_max",
      "key_size_min",   "list_size_avg",    "list_size_max",
      "list_size_min",  "num_blocks",       "num_keys_total",
      "num_keys_valid", "num_values_total", "num_values_valid",
      "num_partitions"};
  return names;
}

Stats Stats::total(const std::vector<Stats>& stats) {
  Stats total;
  for (const auto& stat : stats) {
    if (total.block_size == 0) {
      total.block_size = stat.block_size;
    } else {
      MT_ASSERT_EQ(total.block_size, stat.block_size);
    }
    total.key_size_max = std::max(total.key_size_max, stat.key_size_max);
    if (stat.key_size_min) {
      total.key_size_min = total.key_size_min
                               ? mt::min(total.key_size_min, stat.key_size_min)
                               : stat.key_size_min;
    }
    total.list_size_max = std::max(total.list_size_max, stat.list_size_max);
    if (stat.list_size_min) {
      total.list_size_min =
          total.list_size_min ? mt::min(total.list_size_min, stat.list_size_min)
                              : stat.list_size_min;
    }
    total.num_blocks += stat.num_blocks;
    total.num_keys_total += stat.num_keys_total;
    total.num_keys_valid += stat.num_keys_valid;
    total.num_values_total += stat.num_values_total;
    total.num_values_valid += stat.num_values_valid;
  }
  if (total.num_keys_valid != 0) {
    double key_size_avg = 0;
    double list_size_avg = 0;
    for (const auto& stat : stats) {
      auto w = stat.num_keys_valid / static_cast<double>(total.num_keys_valid);
      key_size_avg += w * stat.key_size_avg;
      list_size_avg += w * stat.list_size_avg;
    }
    total.key_size_avg = std::round(key_size_avg);
    total.list_size_avg = std::round(list_size_avg);
  }
  total.num_partitions = stats.size();
  return total;
}

Stats Stats::max(const std::vector<Stats>& stats) {
  Stats max;
  for (const auto& stat : stats) {
    max.block_size = std::max(max.block_size, stat.block_size);
    max.key_size_avg = std::max(max.key_size_avg, stat.key_size_avg);
    max.key_size_max = std::max(max.key_size_max, stat.key_size_max);
    if (stat.key_size_min) {
      max.key_size_min = std::max(max.key_size_min, stat.key_size_min);
    }
    max.list_size_avg = std::max(max.list_size_avg, stat.list_size_avg);
    max.list_size_max = std::max(max.list_size_max, stat.list_size_max);
    if (stat.list_size_min) {
      max.list_size_min = std::max(max.list_size_min, stat.list_size_min);
    }
    max.num_blocks = std::max(max.num_blocks, stat.num_blocks);
    max.num_keys_total = std::max(max.num_keys_total, stat.num_keys_total);
    max.num_keys_valid = std::max(max.num_keys_valid, stat.num_keys_valid);
    max.num_values_total =
        std::max(max.num_values_total, stat.num_values_total);
    max.num_values_valid =
        std::max(max.num_values_valid, stat.num_values_valid);
  }
  return max;
}

Stats Stats::readFromFile(const boost::filesystem::path& file_path) {
  Stats stats;
  const mt::AutoCloseFile stream = mt::fopen(file_path, "r");
  mt::freadAll(stream.get(), &stats, sizeof stats);
  return stats;
}

void Stats::writeToFile(const boost::filesystem::path& file_path) const {
  const mt::AutoCloseFile stream = mt::fopen(file_path, "w");
  mt::fwriteAll(stream.get(), this, sizeof *this);
}

std::vector<uint64_t> Stats::toVector() const {
  return {block_size,     key_size_avg,   key_size_max,     key_size_min,
          list_size_avg,  list_size_max,  list_size_min,    num_blocks,
          num_keys_total, num_keys_valid, num_values_total, num_values_valid,
          num_partitions};
}

}  // namespace multimap
