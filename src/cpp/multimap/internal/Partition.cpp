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

#include "multimap/internal/Partition.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <boost/filesystem/operations.hpp>  // NOLINT
#include "multimap/internal/Base64.h"
#include "multimap/internal/Locks.h"
#include "multimap/internal/Varint.h"
#include "multimap/thirdparty/mt/check.h"

namespace fs = boost::filesystem;

namespace multimap {
namespace internal {

namespace {

const char* READ_ONLY_VIOLATION = "Attempt to write to read-only partition";

struct SliceEqual {
  explicit SliceEqual(const Slice& value) : value_(value) {}

  bool operator()(const Slice& other) const { return other == value_; }

 private:
  const Slice value_;
};

fs::path getPathOfMapFile(const fs::path& prefix) {
  return prefix.string() + ".map";
}

fs::path getPathOfStatsFile(const fs::path& prefix) {
  return prefix.string() + ".stats";
}

fs::path getPathOfStoreFile(const fs::path& prefix) {
  return prefix.string() + ".store";
}

}  // namespace

size_t Partition::Limits::maxKeySize() {
  return std::numeric_limits<uint32_t>::max();
}

size_t Partition::Limits::maxValueSize() {
  return List::Limits::maxValueSize();
}

Partition::Partition(const fs::path& prefix) : Partition(prefix, Options()) {}

Partition::Partition(const fs::path& prefix, const Options& options)
    : prefix_(prefix) {
  MT_REQUIRE_FALSE(prefix.empty());
  Options store_options;
  store_options.readonly = options.readonly;
  store_options.block_size = options.block_size;
  const fs::path map_file_path = getPathOfMapFile(prefix);
  // TODO(mtrenkmann): Load stats file first and perform check.
  if (fs::is_regular_file(map_file_path)) {
    Bytes key;
    mt::InputStream map_istream = mt::newFileInputStream(map_file_path);
    const fs::path stats_file_path = getPathOfStatsFile(prefix);
    stats_ = Stats::readFromFile(stats_file_path);
    store_options.block_size = stats_.block_size;
    for (size_t i = 0; i != stats_.num_keys_valid; i++) {
      MT_ASSERT_TRUE(readBytesFromStream(map_istream.get(), &key));
      Slice new_key = Slice(key).makeCopy(
          [this](size_t size) { return arena_.allocate(size); });
      List list = List::readFromStream(map_istream.get());
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
  store_ = Store(getPathOfStoreFile(prefix), store_options);
}

Partition::~Partition() {
  if (store_.isReadOnly()) return;

  const fs::path map_file_path = getPathOfMapFile(prefix_);
  const fs::path map_file_path_old = map_file_path.string() + ".old";
  if (fs::is_regular_file(map_file_path)) {
    fs::rename(map_file_path, map_file_path_old);
  }

  List::Stats list_stats;
  mt::OutputStream map_ostream = mt::newFileOutputStream(map_file_path);
  for (const auto& entry : map_) {
    const Slice& key = entry.first;
    List& list = *entry.second;
    if (list.tryFlush(&store_, &list_stats)) {
      // Ok, everything is fine.
    } else {
      const std::string key_as_base64 = Base64::encode(key);
      mt::log() << "The list with the key " << key_as_base64
                << " (Base64) was still locked when shutting down.\n"
                << " The last known state of the list has been safed,"
                << " but ongoing updates, if any, may be lost.\n";
      list.flushUnlocked(&store_, &list_stats);
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
      key.writeToStream(map_ostream.get());
      list.writeToStream(map_ostream.get());
    }
  }
  if (stats_.num_keys_valid) {
    stats_.key_size_avg /= stats_.num_keys_valid;
    stats_.list_size_avg /= stats_.num_keys_valid;
  }
  stats_.block_size = store_.getBlockSize();
  stats_.num_blocks = store_.getNumBlocks();
  stats_.num_keys_total = map_.size();

  stats_.writeToFile(getPathOfStatsFile(prefix_));

  if (fs::is_regular_file(map_file_path_old)) {
    const auto status = fs::remove(map_file_path_old);
    MT_ASSERT_TRUE(status);
  }
}

void Partition::put(const Slice& key, const Slice& value) {
  getListOrCreate(key)->append(value, &store_, &arena_);
}

std::unique_ptr<Iterator> Partition::get(const Slice& key) const {
  const List* list = getList(key);
  return list ? list->newIterator(store_) : Iterator::newEmptyInstance();
}

size_t Partition::remove(const Slice& key) {
  mt::Check::isFalse(store_.isReadOnly(), READ_ONLY_VIOLATION);
  List* list = getList(key);
  return list ? list->clear() : 0;
}

bool Partition::removeFirstEqual(const Slice& key, const Slice& value) {
  return removeFirstMatch(key, SliceEqual(value));
}

size_t Partition::removeAllEqual(const Slice& key, const Slice& value) {
  return removeAllMatches(key, SliceEqual(value));
}

bool Partition::removeFirstMatch(const Slice& key, Predicate predicate) {
  mt::Check::isFalse(store_.isReadOnly(), READ_ONLY_VIOLATION);
  List* list = getList(key);
  return list ? list->removeFirstMatch(predicate, &store_) : false;
}

size_t Partition::removeFirstMatch(Predicate predicate) {
  mt::Check::isFalse(store_.isReadOnly(), READ_ONLY_VIOLATION);
  ReaderLockGuard<boost::shared_mutex> lock(mutex_);
  size_t num_values_removed = 0;
  for (const auto& entry : map_) {
    if (predicate(entry.first)) {
      num_values_removed = entry.second->clear();
      if (num_values_removed != 0) break;
    }
  }
  return num_values_removed;
}

size_t Partition::removeAllMatches(const Slice& key, Predicate predicate) {
  mt::Check::isFalse(store_.isReadOnly(), READ_ONLY_VIOLATION);
  List* list = getList(key);
  return list ? list->removeAllMatches(predicate, &store_) : 0;
}

std::pair<size_t, size_t> Partition::removeAllMatches(Predicate predicate) {
  mt::Check::isFalse(store_.isReadOnly(), READ_ONLY_VIOLATION);
  ReaderLockGuard<boost::shared_mutex> lock(mutex_);
  size_t num_keys_removed = 0;
  size_t num_values_removed = 0;
  for (const auto& entry : map_) {
    if (predicate(entry.first)) {
      const size_t old_size = entry.second->clear();
      if (old_size != 0) {
        num_values_removed += old_size;
        num_keys_removed++;
      }
    }
  }
  return std::make_pair(num_keys_removed, num_values_removed);
}

bool Partition::replaceFirstEqual(const Slice& key, const Slice& old_value,
                                  const Slice& new_value) {
  return replaceFirstMatch(key, [&old_value, &new_value](const Slice& value) {
    return (value == old_value) ? new_value.makeCopy() : Bytes();
  });
}

size_t Partition::replaceAllEqual(const Slice& key, const Slice& old_value,
                                  const Slice& new_value) {
  return replaceAllMatches(key, [&old_value, &new_value](const Slice& value) {
    return (value == old_value) ? new_value.makeCopy() : Bytes();
  });
}

bool Partition::replaceFirstMatch(const Slice& key, Function map) {
  mt::Check::isFalse(store_.isReadOnly(), READ_ONLY_VIOLATION);
  List* list = getList(key);
  return list ? list->replaceFirstMatch(map, &store_, &arena_) : false;
}

size_t Partition::replaceAllMatches(const Slice& key, Function map) {
  mt::Check::isFalse(store_.isReadOnly(), READ_ONLY_VIOLATION);
  List* list = getList(key);
  return list ? list->replaceAllMatches(map, &store_, &arena_) : 0;
}

size_t Partition::replaceAllMatches(const Slice& key, Function2 map) {
  mt::Check::isFalse(store_.isReadOnly(), READ_ONLY_VIOLATION);
  List* list = getList(key);
  return list ? list->replaceAllMatches(map, &store_, &arena_) : 0;
}

void Partition::forEachKey(Procedure process) const {
  ReaderLockGuard<boost::shared_mutex> lock(mutex_);
  for (const auto& entry : map_) {
    if (!entry.second->empty()) {
      process(entry.first);
    }
  }
}

void Partition::forEachValue(const Slice& key, Procedure process) const {
  if (const List* list = getList(key)) {
    list->forEachValue(process, store_);
  }
}

void Partition::forEachEntry(BinaryProcedure process) const {
  ReaderLockGuard<boost::shared_mutex> lock(mutex_);
  for (const auto& entry : map_) {
    auto iter = entry.second->newIterator(store_);
    if (iter->hasNext()) {
      process(entry.first, iter.get());
    }
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
  stats.block_size = store_.getBlockSize();
  stats.num_blocks = store_.getNumBlocks();
  stats.num_keys_total = map_.size();
  return stats;
}

void Partition::forEachEntry(const fs::path& prefix, BinaryProcedure process) {
  const Stats stats = Stats::readFromFile(getPathOfStatsFile(prefix));

  Bytes key;
  Options store_options;
  store_options.readonly = true;
  store_options.block_size = stats.block_size;
  Store store(getPathOfStoreFile(prefix), store_options);
  mt::InputStream istream = mt::newFileInputStream(getPathOfMapFile(prefix));
  for (size_t i = 0; i != stats.num_keys_valid; i++) {
    MT_ASSERT_TRUE(readBytesFromStream(istream.get(), &key));
    const List list = List::readFromStream(istream.get());
    const auto iter = list.newIterator(store);
    process(key, iter.get());
  }
}

Stats Partition::stats(const fs::path& prefix) {
  return Stats::readFromFile(getPathOfStatsFile(prefix));
}

List* Partition::getList(const Slice& key) const {
  ReaderLockGuard<boost::shared_mutex> lock(mutex_);
  const auto iter = map_.find(key);
  return (iter != map_.end()) ? iter->second.get() : nullptr;
}

List* Partition::getListOrCreate(const Slice& key) {
  mt::Check::isFalse(store_.isReadOnly(), READ_ONLY_VIOLATION);
  MT_REQUIRE_LE(key.size(), Limits::maxKeySize());
  WriterLockGuard<boost::shared_mutex> lock(mutex_);
  auto iter = map_.find(key);
  if (iter == map_.end()) {
    const Slice new_key =
        key.makeCopy([this](size_t size) { return arena_.allocate(size); });
    iter = map_.emplace(new_key, std::unique_ptr<List>(new List())).first;
  }
  return iter->second.get();
}

}  // namespace internal
}  // namespace multimap
