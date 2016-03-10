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

#include "multimap/internal/Partition.hpp"

#include <cmath>
#include <boost/filesystem/operations.hpp>
#include "multimap/internal/Base64.hpp"

namespace multimap {
namespace internal {

const char* Partition::ATTEMPT_TO_MODIFY_READ_ONLY_PARTITION =
    "Attempt to modify read-only partition";

uint32_t Partition::Limits::maxKeySize() { return Varint::Limits::MAX_N4; }

uint32_t Partition::Limits::maxValueSize() {
  return List::Limits::maxValueSize();
}

Partition::Partition(const std::string& prefix)
    : Partition(prefix, Options()) {}

Partition::Partition(const std::string& prefix, const Options& options)
    : prefix_(prefix) {
  MT_REQUIRE_FALSE(prefix.empty());
  Store::Options store_options;
  store_options.readonly = options.readonly;
  store_options.block_size = options.block_size;
  const auto map_filename = getNameOfMapFile(prefix);
  if (boost::filesystem::is_regular_file(map_filename)) {
    Bytes key;
    const auto map_stream = mt::open(map_filename, "r");
    const auto stats_filename = getNameOfStatsFile(prefix);
    stats_ = Stats::readFromFile(stats_filename);
    store_options.block_size = stats_.block_size;
    for (size_t i = 0; i != stats_.num_keys_valid; i++) {
      MT_ASSERT_TRUE(readBytesFromStream(map_stream.get(), &key));
      const Range new_key = Range(key).makeCopy(
          [this](size_t size) { return arena_.allocate(size); });
      List list = List::readFromStream(map_stream.get());
      stats_.num_values_total -= list.getStatsUnlocked().num_values_total;
      stats_.num_values_valid -= list.getStatsUnlocked().num_values_valid();
      map_.emplace(new_key, std::unique_ptr<List>(new List(std::move(list))));
    }

    // Reset stats, but preserve number of total and valid values.
    Stats stats;
    stats.num_values_total = stats_.num_values_total;
    stats.num_values_valid = stats_.num_values_valid;
    stats_ = stats;
  }
  store_.reset(new Store(getNameOfStoreFile(prefix), store_options));
}

Partition::~Partition() {
  if (isReadOnly()) return;

  const auto map_filename = getNameOfMapFile(prefix_);
  const auto map_filename_old = map_filename + ".old";
  if (boost::filesystem::is_regular_file(map_filename)) {
    boost::filesystem::rename(map_filename, map_filename_old);
  }

  List::Stats list_stats;
  const auto map_stream = mt::open(map_filename, "w");
  for (const auto& entry : map_) {
    auto& key = entry.first;
    auto& list = *entry.second;
    if (list.tryFlush(store_.get(), &list_stats)) {
      // Ok, everything is fine.
    } else {
      const auto key_as_base64 = Base64::encode(key);
      mt::log() << "The list with the key " << key_as_base64
                << " (Base64) was still locked when shutting down.\n"
                << " The last known state of the list has been safed,"
                << " but ongoing updates, if any, may be lost.\n";
      list.flushUnlocked(store_.get(), &list_stats);
    }
    stats_.num_values_total += list_stats.num_values_total;
    stats_.num_values_valid += list_stats.num_values_valid();
    const auto list_size = list_stats.num_values_valid();
    if (list_size != 0) {
      ++stats_.num_keys_valid;
      stats_.key_size_avg += key.size();
      stats_.key_size_max = mt::max(stats_.key_size_max, key.size());
      stats_.key_size_min = stats_.key_size_min
                                ? mt::min(stats_.key_size_min, key.size())
                                : key.size();
      stats_.list_size_avg += list_size;
      stats_.list_size_max = mt::max(stats_.list_size_max, list_size);
      stats_.list_size_min = stats_.list_size_min
                                 ? mt::min(stats_.list_size_min, list_size)
                                 : list_size;
      key.writeToStream(map_stream.get());
      list.writeToStream(map_stream.get());
    }
  }
  if (stats_.num_keys_valid) {
    stats_.key_size_avg /= stats_.num_keys_valid;
    stats_.list_size_avg /= stats_.num_keys_valid;
  }
  stats_.block_size = store_->getBlockSize();
  stats_.num_blocks = store_->getNumBlocks();
  stats_.num_keys_total = map_.size();

  stats_.writeToFile(getNameOfStatsFile(prefix_));

  if (boost::filesystem::is_regular_file(map_filename_old)) {
    const auto status = boost::filesystem::remove(map_filename_old);
    MT_ASSERT_TRUE(status);
  }
}

Stats Partition::getStats() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  Stats stats = stats_;
  List::Stats list_stats;
  for (const auto& entry : map_) {
    if (entry.second->tryGetStats(&list_stats)) {
      stats.num_values_total += list_stats.num_values_total;
      stats.num_values_valid += list_stats.num_values_valid();
      const auto list_size = list_stats.num_values_valid();
      if (list_size != 0) {
        const auto& key = entry.first;
        stats.num_keys_valid++;
        stats.key_size_avg += key.size();
        stats.key_size_max = mt::max(stats.key_size_max, key.size());
        stats.key_size_min = stats.key_size_min
                                 ? mt::min(stats.key_size_min, key.size())
                                 : key.size();
        stats.list_size_avg += list_size;
        stats.list_size_max = mt::max(stats.list_size_max, list_size);
        stats.list_size_min = stats.list_size_min
                                  ? mt::min(stats.list_size_min, list_size)
                                  : list_size;
      }
    }
  }
  if (stats.num_keys_valid) {
    stats.key_size_avg /= stats.num_keys_valid;
    stats.list_size_avg /= stats.num_keys_valid;
  }
  stats.block_size = store_->getBlockSize();
  stats.num_blocks = store_->getNumBlocks();
  stats.num_keys_total = map_.size();
  return stats;
}

std::string Partition::getNameOfMapFile(const std::string& prefix) {
  return prefix + ".map";
}

std::string Partition::getNameOfStatsFile(const std::string& prefix) {
  return prefix + ".stats";
}

std::string Partition::getNameOfStoreFile(const std::string& prefix) {
  return prefix + ".store";
}

}  // namespace internal
}  // namespace multimap
