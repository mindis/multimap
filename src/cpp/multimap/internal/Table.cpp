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

#include "multimap/internal/Table.hpp"

#include <cmath>
#include <boost/filesystem/operations.hpp>
#include "multimap/internal/Base64.hpp"

namespace multimap {
namespace internal {

uint32_t Table::Limits::maxKeySize() { return Varint::Limits::MAX_N4; }

uint32_t Table::Limits::maxValueSize() { return List::Limits::maxValueSize(); }

const std::vector<std::string>& Table::Stats::names() {
  static std::vector<std::string> names = {
    "block_size",     "key_size_avg",   "key_size_max",     "key_size_min",
    "list_size_avg",  "list_size_max",  "list_size_min",    "num_blocks",
    "num_keys_total", "num_keys_valid", "num_values_total", "num_values_valid",
    "num_partitions"
  };
  return names;
}

Table::Stats Table::Stats::readFromFile(const boost::filesystem::path& file) {
  Stats stats;
  const auto stream = mt::fopen(file, "r");
  mt::fread(stream.get(), &stats, sizeof stats);
  return stats;
}

void Table::Stats::writeToFile(const boost::filesystem::path& file) const {
  const auto stream = mt::fopen(file.c_str(), "w");
  mt::fwrite(stream.get(), this, sizeof *this);
}

std::vector<uint64_t> Table::Stats::toVector() const {
  return { block_size,     key_size_avg,   key_size_max,     key_size_min,
           list_size_avg,  list_size_max,  list_size_min,    num_blocks,
           num_keys_total, num_keys_valid, num_values_total, num_values_valid,
           num_partitions };
}

Table::Stats Table::Stats::total(const std::vector<Stats>& stats) {
  Table::Stats total;
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

Table::Stats Table::Stats::max(const std::vector<Stats>& stats) {
  Table::Stats max;
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

Table::Table(const boost::filesystem::path& file_prefix)
    : Table(file_prefix, Options()) {}

Table::Table(const boost::filesystem::path& file_prefix, const Options& options)
    : prefix_(file_prefix) {
  Store::Options store_options;
  const auto stats_filename = getNameOfStatsFile(prefix_.string());
  if (boost::filesystem::is_regular_file(stats_filename)) {
    stats_ = Stats::readFromFile(stats_filename);
    store_options.block_size = stats_.block_size;
    const auto keys_filename = getNameOfKeysFile(prefix_.string());
    const auto keys_input = mt::fopen(keys_filename, "r");
    for (size_t i = 0; i != stats_.num_keys_valid; ++i) {
      const auto entry = Entry::readFromStream(keys_input.get(), &arena_);
      map_[entry.key()].reset(new List(entry.head()));
      stats_.num_values_total -= entry.head().num_values_total;
      stats_.num_values_valid -= entry.head().num_values_valid();
    }

    // Reset stats, but preserve number of total and valid values.
    Stats stats;
    stats.num_values_total = stats_.num_values_total;
    stats.num_values_valid = stats_.num_values_valid;
    stats_ = stats;

  } else {
    mt::Check::isTrue(options.create_if_missing,
                      "Table with prefix '%s' does not exist",
                      boost::filesystem::absolute(file_prefix).c_str());
  }

  store_options.readonly = options.readonly;
  store_options.buffer_size = options.buffer_size;
  store_.reset(new Store(getNameOfValuesFile(prefix_.string()), store_options));
}

Table::~Table() {
  if (!prefix_.empty() && !isReadOnly()) {
    const auto keys_file = getNameOfKeysFile(prefix_.string());
    const auto old_keys_file = keys_file + ".old";
    if (boost::filesystem::is_regular_file(keys_file)) {
      boost::filesystem::rename(keys_file, old_keys_file);
    }

    const auto stream = mt::fopen(keys_file, "w");
    for (const auto& entry : map_) {
      const auto& key = entry.first;
      const auto& list = entry.second.get();
      if (list->is_locked()) {
        const auto key_as_base64 = Base64::encode(key);
        mt::log() << "The list with the key " << key_as_base64
                  << " (Base64) was still locked when shutting down."
                  << " Recent updates of the list may be lost.\n";
      }
      // We do not skip or even throw if a list is still locked to prevent data
      // loss. However, this causes a race which could let the program crash...
      list->flush(store_.get());
      stats_.num_values_total += list->head().num_values_total;
      stats_.num_values_valid += list->head().num_values_valid();
      if (!list->empty()) {
        ++stats_.num_keys_valid;
        stats_.key_size_avg += key.size();
        stats_.key_size_max = mt::max(stats_.key_size_max, key.size());
        stats_.key_size_min = stats_.key_size_min
                                  ? mt::min(stats_.key_size_min, key.size())
                                  : key.size();
        stats_.list_size_avg += list->size();
        stats_.list_size_max = mt::max(stats_.list_size_max, list->size());
        stats_.list_size_min = stats_.list_size_min
                                   ? mt::min(stats_.list_size_min, list->size())
                                   : list->size();
        Entry(key, list->head()).writeToStream(stream.get());
      }
    }
    if (stats_.num_keys_valid) {
      stats_.key_size_avg /= stats_.num_keys_valid;
      stats_.list_size_avg /= stats_.num_keys_valid;
    }
    stats_.block_size = store_->getBlockSize();
    stats_.num_blocks = store_->getNumBlocks();
    stats_.num_keys_total = map_.size();

    stats_.writeToFile(getNameOfStatsFile(prefix_.string()));

    if (boost::filesystem::is_regular_file(old_keys_file)) {
      const auto status = boost::filesystem::remove(old_keys_file);
      MT_ASSERT_TRUE(status);
    }
  }
}

Table::Stats Table::getStats() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  Stats stats = stats_;
  for (const auto& entry : map_) {
    if (auto list = SharedList(*entry.second, *store_, std::try_to_lock)) {
      stats.num_values_total += list.head().num_values_total;
      stats.num_values_valid += list.head().num_values_valid();
      if (!list.empty()) {
        const auto& key = entry.first;
        ++stats.num_keys_valid;
        stats.key_size_avg += key.size();
        stats.key_size_max = mt::max(stats.key_size_max, key.size());
        stats.key_size_min = stats.key_size_min
                                 ? mt::min(stats.key_size_min, key.size())
                                 : key.size();
        stats.list_size_avg += list.size();
        stats.list_size_max = mt::max(stats.list_size_max, list.size());
        stats.list_size_min = stats.list_size_min
                                  ? mt::min(stats.list_size_min, list.size())
                                  : list.size();
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

std::string Table::getNameOfKeysFile(const std::string& prefix) {
  return prefix + ".keys";
}

std::string Table::getNameOfStatsFile(const std::string& prefix) {
  return prefix + ".stats";
}

std::string Table::getNameOfValuesFile(const std::string& prefix) {
  return prefix + ".values";
}

Table::Entry Table::Entry::readFromStream(std::FILE* stream, Arena* arena) {
  uint32_t key_size;
  mt::fread(stream, &key_size, sizeof key_size);
  const auto key_data = arena->allocate(key_size);
  mt::fread(stream, key_data, key_size);
  auto head = List::Head::readFromStream(stream);
  return Entry(Bytes(key_data, key_size), std::move(head));
}

void Table::Entry::writeToStream(std::FILE* stream) const {
  MT_REQUIRE_LE(key().size(), Table::Limits::maxKeySize());
  const uint32_t key_size = key().size();
  mt::fwrite(stream, &key_size, sizeof key_size);
  mt::fwrite(stream, key().data(), key().size());
  head().writeToStream(stream);
}

SharedList Table::getSharedList(const Bytes& key) const {
  List* list = nullptr;
  {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    const auto iter = map_.find(key);
    if (iter != map_.end()) {
      list = iter->second.get();
    }
  }
  // `mutex_` is unlocked now.
  return list ? SharedList(*list, *store_) : SharedList();
}

UniqueList Table::getUniqueList(const Bytes& key) {
  List* list = nullptr;
  {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    const auto iter = map_.find(key);
    if (iter != map_.end()) {
      list = iter->second.get();
    }
  }
  // `mutex_` is unlocked now.
  return list ? UniqueList(list, store_.get(), &arena_) : UniqueList();
}

UniqueList Table::getOrCreateUniqueList(const Bytes& key) {
  MT_REQUIRE_LE(key.size(), Limits::maxKeySize());
  List* list = nullptr;
  {
    std::lock_guard<boost::shared_mutex> lock(mutex_);
    const auto emplace_result = map_.emplace(key, std::unique_ptr<List>());
    if (emplace_result.second) {
      // Overrides the inserted key with a new deep copy.
      const auto new_key_data = arena_.allocate(key.size());
      std::memcpy(new_key_data, key.data(), key.size());
      const auto old_key_ptr = const_cast<Bytes*>(&emplace_result.first->first);
      *old_key_ptr = Bytes(new_key_data, key.size());
      emplace_result.first->second.reset(new List());
    }
    const auto iter = emplace_result.first;
    list = iter->second.get();
  }
  // `mutex_` is unlocked now.
  return UniqueList(list, store_.get(), &arena_);
}

} // namespace internal
} // namespace multimap
