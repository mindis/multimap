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

#ifndef MULTIMAP_INTERNAL_PARTITION_HPP_INCLUDED
#define MULTIMAP_INTERNAL_PARTITION_HPP_INCLUDED

#include <memory>
#include <unordered_map>
#include <vector>
#include <boost/filesystem/path.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "multimap/internal/Arena.hpp"
#include "multimap/internal/List.hpp"
#include "multimap/internal/Locks.hpp"
#include "multimap/internal/Stats.hpp"
#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/Iterator.hpp"

namespace multimap {
namespace internal {

class Partition {
  static const char* ATTEMPT_TO_MODIFY_READ_ONLY_PARTITION;

 public:
  // ---------------------------------------------------------------------------
  // Member types
  // ---------------------------------------------------------------------------

  struct Limits {
    static uint32_t maxKeySize();
    static uint32_t maxValueSize();
  };

  struct Options {
    uint32_t block_size = 512;
    bool readonly = false;
  };

  // ---------------------------------------------------------------------------
  // Member functions
  // ---------------------------------------------------------------------------

  explicit Partition(const std::string& prefix);

  Partition(const std::string& prefix, const Options& options);

  ~Partition();

  void put(const Range& key, const Range& value) {
    mt::Check::isFalse(isReadOnly(), ATTEMPT_TO_MODIFY_READ_ONLY_PARTITION);
    getListOrCreate(key)->append(value, store_.get(), &arena_);
  }

  template <typename InputIter>
  void put(const Range& key, InputIter first, InputIter last) {
    mt::Check::isFalse(isReadOnly(), ATTEMPT_TO_MODIFY_READ_ONLY_PARTITION);
    getListOrCreate(key)->append(first, last, store_.get(), &arena_);
  }

  std::unique_ptr<Iterator> get(const Range& key) const {
    const auto list = getList(key);
    return list ? list->newIterator(*store_) : Iterator::newEmptyInstance();
  }

  bool contains(const Range& key) const {
    const auto list = getList(key);
    return list ? !list->empty() : false;
  }

  uint32_t remove(const Range& key) {
    mt::Check::isFalse(isReadOnly(), ATTEMPT_TO_MODIFY_READ_ONLY_PARTITION);
    const auto list = getList(key);
    return list ? list->clear() : 0;
  }

  bool removeFirstEqual(const Range& key, const Range& value) {
    return removeFirstMatch(key, Equal(value));
  }

  uint32_t removeAllEqual(const Range& key, const Range& value) {
    return removeAllMatches(key, Equal(value));
  }

  template <typename Predicate>
  bool removeFirstMatch(const Range& key, Predicate predicate) {
    mt::Check::isFalse(isReadOnly(), ATTEMPT_TO_MODIFY_READ_ONLY_PARTITION);
    const auto list = getList(key);
    return list ? list->removeFirstMatch(predicate, store_.get()) : false;
  }

  template <typename Predicate>
  uint32_t removeFirstMatch(Predicate predicate) {
    mt::Check::isFalse(isReadOnly(), ATTEMPT_TO_MODIFY_READ_ONLY_PARTITION);
    ReaderLockGuard<boost::shared_mutex> lock(mutex_);
    uint32_t num_values_removed = 0;
    for (const auto& entry : map_) {
      if (predicate(entry.first)) {
        num_values_removed = entry.second->clear();
        if (num_values_removed != 0) break;
      }
    }
    return num_values_removed;
  }

  template <typename Predicate>
  uint32_t removeAllMatches(const Range& key, Predicate predicate) {
    mt::Check::isFalse(isReadOnly(), ATTEMPT_TO_MODIFY_READ_ONLY_PARTITION);
    const auto list = getList(key);
    return list ? list->removeAllMatches(predicate, store_.get()) : 0;
  }

  template <typename Predicate>
  std::pair<uint32_t, uint64_t> removeAllMatches(Predicate predicate) {
    mt::Check::isFalse(isReadOnly(), ATTEMPT_TO_MODIFY_READ_ONLY_PARTITION);
    ReaderLockGuard<boost::shared_mutex> lock(mutex_);
    uint32_t num_keys_removed = 0;
    uint64_t num_values_removed = 0;
    for (const auto& entry : map_) {
      if (predicate(entry.first)) {
        const auto old_size = entry.second->clear();
        if (old_size != 0) {
          num_values_removed += old_size;
          num_keys_removed++;
        }
      }
    }
    return std::make_pair(num_keys_removed, num_values_removed);
  }

  bool replaceFirstEqual(const Range& key, const Range& old_value,
                         const Range& new_value) {
    return replaceFirstMatch(key, [&old_value, &new_value](const Range& value) {
      return (value == old_value) ? new_value.makeCopy() : Bytes();
    });
  }

  uint32_t replaceAllEqual(const Range& key, const Range& old_value,
                           const Range& new_value) {
    return replaceAllMatches(key, [&old_value, &new_value](const Range& value) {
      return (value == old_value) ? new_value.makeCopy() : Bytes();
    });
  }

  template <typename Function>
  bool replaceFirstMatch(const Range& key, Function map) {
    mt::Check::isFalse(isReadOnly(), ATTEMPT_TO_MODIFY_READ_ONLY_PARTITION);
    const auto list = getList(key);
    return list ? list->replaceFirstMatch(map, store_.get(), &arena_) : false;
  }

  template <typename Function>
  uint32_t replaceAllMatches(const Range& key, Function map) {
    mt::Check::isFalse(isReadOnly(), ATTEMPT_TO_MODIFY_READ_ONLY_PARTITION);
    const auto list = getList(key);
    return list ? list->replaceAllMatches(map, store_.get(), &arena_) : 0;
  }

  template <typename Procedure>
  void forEachKey(Procedure process) const {
    ReaderLockGuard<boost::shared_mutex> lock(mutex_);
    for (const auto& entry : map_) {
      if (!entry.second->empty()) {
        process(entry.first);
      }
    }
  }

  template <typename Procedure>
  void forEachValue(const Range& key, Procedure process) const {
    if (auto list = getList(key)) {
      list->forEachValue(process, *store_);
    }
  }

  template <typename BinaryProcedure>
  void forEachEntry(BinaryProcedure process) const {
    ReaderLockGuard<boost::shared_mutex> lock(mutex_);
    store_->adviseAccessPattern(Store::AccessPattern::WILLNEED);
    for (const auto& entry : map_) {
      auto iter = entry.second->newIterator(*store_);
      if (iter->hasNext()) {
        process(entry.first, iter.get());
      }
    }
    store_->adviseAccessPattern(Store::AccessPattern::NORMAL);
  }

  Stats getStats() const;
  // Returns various statistics about the partition.
  // The data is collected upon request and triggers a full partition scan.

  bool isReadOnly() const { return store_->isReadOnly(); }

  uint32_t getBlockSize() const { return store_->getBlockSize(); }

  // ---------------------------------------------------------------------------
  // Static member functions
  // ---------------------------------------------------------------------------

  template <typename BinaryProcedure>
  static void forEachEntry(const std::string& prefix, BinaryProcedure process) {
    const auto stats = Stats::readFromFile(getNameOfStatsFile(prefix));

    Store::Options store_options;
    store_options.readonly = true;
    store_options.block_size = stats.block_size;
    Store store(getNameOfStoreFile(prefix), store_options);
    store.adviseAccessPattern(Store::AccessPattern::WILLNEED);

    Bytes key;
    const auto stream = mt::open(getNameOfMapFile(prefix), "r");
    for (size_t i = 0; i != stats.num_keys_valid; i++) {
      MT_ASSERT_TRUE(readBytesFromStream(stream.get(), &key));
      const List list = List::readFromStream(stream.get());
      const auto iter = list.newIterator(store);
      process(key, iter.get());
    }
  }

  static std::string getNameOfMapFile(const std::string& prefix);
  static std::string getNameOfStatsFile(const std::string& prefix);
  static std::string getNameOfStoreFile(const std::string& prefix);

 private:
  struct Equal {
    explicit Equal(const Range& value) : value_(value) {}

    bool operator()(const Range& other) const { return other == value_; }

   private:
    const Range value_;
  };

  List* getList(const Range& key) const {
    ReaderLockGuard<boost::shared_mutex> lock(mutex_);
    const auto iter = map_.find(key);
    return (iter != map_.end()) ? iter->second.get() : nullptr;
  }

  List* getListOrCreate(const Range& key) {
    MT_REQUIRE_LE(key.size(), Limits::maxKeySize());
    WriterLockGuard<boost::shared_mutex> lock(mutex_);
    const auto emplace_result = map_.emplace(key, std::unique_ptr<List>());
    if (emplace_result.second) {
      // Overrides the inserted key with a new deep copy.
      byte* new_key_data = arena_.allocate(key.size());
      std::memcpy(new_key_data, key.begin(), key.size());
      Range* old_key_ptr = const_cast<Range*>(&emplace_result.first->first);
      *old_key_ptr = Range(new_key_data, key.size());
      emplace_result.first->second.reset(new List());
    }
    return emplace_result.first->second.get();
  }

  mutable boost::shared_mutex mutex_;
  std::unordered_map<Range, std::unique_ptr<List>> map_;
  std::unique_ptr<Store> store_;
  std::string prefix_;
  Arena arena_;
  Stats stats_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_PARTITION_HPP_INCLUDED
