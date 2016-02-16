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

#include "multimap/internal/MapPartition.hpp"

#include <cmath>
#include <boost/filesystem/operations.hpp>
#include "multimap/internal/Base64.hpp"

namespace multimap {
namespace internal {

uint32_t MapPartition::Limits::maxKeySize() { return Varint::Limits::MAX_N4; }

uint32_t MapPartition::Limits::maxValueSize() {
  return List::Limits::maxValueSize();
}

MapPartition::MapPartition(const boost::filesystem::path& file_prefix)
    : MapPartition(file_prefix, Options()) {}

MapPartition::MapPartition(const boost::filesystem::path& file_prefix,
                           const Options& options)
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
                      "MapPartition with prefix '%s' does not exist",
                      boost::filesystem::absolute(file_prefix).c_str());
  }

  store_options.readonly = options.readonly;
  store_options.buffer_size = options.buffer_size;
  store_.reset(new Store(getNameOfValuesFile(prefix_.string()), store_options));
}

MapPartition::~MapPartition() {
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

Stats MapPartition::getStats() const {
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

std::string MapPartition::getNameOfKeysFile(const std::string& prefix) {
  return prefix + ".keys";
}

std::string MapPartition::getNameOfStatsFile(const std::string& prefix) {
  return prefix + ".stats";
}

std::string MapPartition::getNameOfValuesFile(const std::string& prefix) {
  return prefix + ".values";
}

MapPartition::Entry MapPartition::Entry::readFromStream(std::FILE* stream,
                                                        Arena* arena) {
  uint32_t key_size;
  mt::fread(stream, &key_size, sizeof key_size);
  const auto key_data = arena->allocate(key_size);
  mt::fread(stream, key_data, key_size);
  auto head = List::Head::readFromStream(stream);
  return Entry(Bytes(key_data, key_size), std::move(head));
}

void MapPartition::Entry::writeToStream(std::FILE* stream) const {
  MT_REQUIRE_LE(key().size(), MapPartition::Limits::maxKeySize());
  const uint32_t key_size = key().size();
  mt::fwrite(stream, &key_size, sizeof key_size);
  mt::fwrite(stream, key().data(), key().size());
  head().writeToStream(stream);
}

SharedList MapPartition::getSharedList(const Bytes& key) const {
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

ExclusiveList MapPartition::getUniqueList(const Bytes& key) {
  List* list = nullptr;
  {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    const auto iter = map_.find(key);
    if (iter != map_.end()) {
      list = iter->second.get();
    }
  }
  // `mutex_` is unlocked now.
  return list ? ExclusiveList(list, store_.get(), &arena_) : ExclusiveList();
}

ExclusiveList MapPartition::getOrCreateUniqueList(const Bytes& key) {
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
  return ExclusiveList(list, store_.get(), &arena_);
}

}  // namespace internal
}  // namespace multimap
