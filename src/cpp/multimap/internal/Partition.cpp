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
#include "multimap/internal/Locks.hpp"
#include "multimap/internal/Varint.hpp"

namespace multimap {
namespace internal {

namespace {

const char* READ_ONLY_VIOLATION = "Attempt to write to read-only partition";

struct Equal {
  explicit Equal(const Slice& value) : value_(value) {}

  bool operator()(const Slice& other) const { return other == value_; }

 private:
  const Slice value_;
};

std::string getNameOfMapFile(const std::string& prefix) {
  return prefix + ".map";
}

std::string getNameOfStatsFile(const std::string& prefix) {
  return prefix + ".stats";
}

std::string getNameOfStoreFile(const std::string& prefix) {
  return prefix + ".store";
}

}  // namespace

size_t Partition::Limits::maxKeySize() { return Varint::Limits::MAX_N4; }

size_t Partition::Limits::maxValueSize() {
  return List::Limits::maxValueSize();
}

Partition::Partition(const std::string& prefix)
    : Partition(prefix, Options()) {}

Partition::Partition(const std::string& prefix, const Options& options)
    : prefix_(prefix) {
  MT_REQUIRE_FALSE(prefix.empty());
  Options store_options;
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
      Slice new_key = Slice(key).makeCopy(
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
  store_ = Store(getNameOfStoreFile(prefix), store_options);
}

Partition::~Partition() {
  if (store_.isReadOnly()) return;

  const auto map_filename = getNameOfMapFile(prefix_);
  const auto map_filename_old = map_filename + ".old";
  if (boost::filesystem::is_regular_file(map_filename)) {
    boost::filesystem::rename(map_filename, map_filename_old);
  }

  List::Stats list_stats;
  mt::AutoCloseFile map_stream = mt::open(map_filename, "w");
  for (const auto& entry : map_) {
    auto& key = entry.first;
    auto& list = *entry.second;
    if (list.tryFlush(&store_, &list_stats)) {
      // Ok, everything is fine.
    } else {
      const auto key_as_base64 = Base64::encode(key);
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
      key.writeToStream(map_stream.get());
      list.writeToStream(map_stream.get());
    }
  }
  if (stats_.num_keys_valid) {
    stats_.key_size_avg /= stats_.num_keys_valid;
    stats_.list_size_avg /= stats_.num_keys_valid;
  }
  stats_.block_size = store_.getBlockSize();
  stats_.num_blocks = store_.getNumBlocks();
  stats_.num_keys_total = map_.size();

  stats_.writeToFile(getNameOfStatsFile(prefix_));

  if (boost::filesystem::is_regular_file(map_filename_old)) {
    const auto status = boost::filesystem::remove(map_filename_old);
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
  return removeFirstMatch(key, Equal(value));
}

size_t Partition::removeAllEqual(const Slice& key, const Slice& value) {
  return removeAllMatches(key, Equal(value));
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

void Partition::forEachEntry(const std::string& prefix,
                             BinaryProcedure process) {
  const auto stats = Stats::readFromFile(getNameOfStatsFile(prefix));

  Bytes key;
  Options store_options;
  store_options.readonly = true;
  store_options.block_size = stats.block_size;
  Store store(getNameOfStoreFile(prefix), store_options);
  const auto stream = mt::open(getNameOfMapFile(prefix), "r");
  for (size_t i = 0; i != stats.num_keys_valid; i++) {
    MT_ASSERT_TRUE(readBytesFromStream(stream.get(), &key));
    const List list = List::readFromStream(stream.get());
    const auto iter = list.newIterator(store);
    process(key, iter.get());
  }
}

Stats Partition::stats(const std::string& prefix) {
  return Stats::readFromFile(getNameOfStatsFile(prefix));
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
