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
#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/Iterator.hpp"
#include "multimap/Stats.hpp"

namespace multimap {
namespace internal {

class Partition {
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

  void put(const Slice& key, const Slice& value) {
    getListOrCreate(key)->append(value, store_.get(), &arena_);
  }

  template <typename InputIter>
  void put(const Slice& key, InputIter first, InputIter last) {
    getListOrCreate(key)->append(first, last, store_.get(), &arena_);
  }

  std::unique_ptr<Iterator> get(const Slice& key) const {
//    const auto list = getList(key);
//    return list ? list->newIterator(*store_) : Iterator::newEmptyInstance();
    return Iterator::newEmptyInstance();
  }

  bool contains(const Slice& key) const {
//    const auto list = getList(key);
//    return list ? !list->empty() : false;
    return false;
  }

  uint32_t remove(const Slice& key) {
//    const auto list = getList(key);
//    return list ? list->clear() : 0;
    return 0;
  }

  bool removeFirstEqual(const Slice& key, const Slice& value) {
    return removeFirstMatch(key, Equal(value));
  }

  uint32_t removeAllEqual(const Slice& key, const Slice& value) {
    return removeAllMatches(key, Equal(value));
  }

  template <typename Predicate>
  bool removeFirstMatch(const Slice& key, Predicate predicate) {
//    const auto list = getList(key);
//    return list ? list->removeFirstMatch(predicate, store_.get()) : false;
    return false;
  }

  template <typename Predicate>
  uint32_t removeFirstMatch(Predicate predicate) {
    ReaderLockGuard<boost::shared_mutex> lock(mutex_);
    uint32_t num_values_removed = 0;
//    for (const auto& entry : map_) {
//      if (predicate(entry.first)) {
//        num_values_removed = entry.second->clear();
//        if (num_values_removed != 0) break;
//      }
//    }
    return num_values_removed;
  }

  template <typename Predicate>
  uint32_t removeAllMatches(const Slice& key, Predicate predicate) {
//    const auto list = getList(key);
//    return list ? list->removeAllMatches(predicate, store_.get()) : 0;
    return 0;
  }

  template <typename Predicate>
  std::pair<uint32_t, uint64_t> removeAllMatches(Predicate predicate) {
    ReaderLockGuard<boost::shared_mutex> lock(mutex_);
    uint32_t num_keys_removed = 0;
    uint64_t num_values_removed = 0;
//    for (const auto& entry : map_) {
//      if (predicate(entry.first)) {
//        const auto old_size = entry.second->clear();
//        if (old_size != 0) {
//          num_values_removed += old_size;
//          num_keys_removed++;
//        }
//      }
//    }
    return std::make_pair(num_keys_removed, num_values_removed);
  }

  bool replaceFirstEqual(const Slice& key, const Slice& old_value,
                         const Slice& new_value) {
    return replaceFirstMatch(key, [&old_value, &new_value](const Slice& value) {
      return (value == old_value) ? new_value.makeCopy() : Bytes();
    });
  }

  uint32_t replaceAllEqual(const Slice& key, const Slice& old_value,
                           const Slice& new_value) {
    return replaceAllMatches(key, [&old_value, &new_value](const Slice& value) {
      return (value == old_value) ? new_value.makeCopy() : Bytes();
    });
  }

  template <typename Function>
  bool replaceFirstMatch(const Slice& key, Function map) {
//    const auto list = getList(key);
//    return list ? list->replaceFirstMatch(map, store_.get(), &arena_) : false;
    return false;
  }

  template <typename Function>
  uint32_t replaceAllMatches(const Slice& key, Function map) {
//    const auto list = getList(key);
//    return list ? list->replaceAllMatches(map, store_.get(), &arena_) : 0;
    return 0;
  }

  template <typename Procedure>
  void forEachKey(Procedure process) const {
    ReaderLockGuard<boost::shared_mutex> lock(mutex_);
//    for (const auto& entry : map_) {
//      if (!entry.second->empty()) {
//        process(entry.first);
//      }
//    }
  }

  template <typename Procedure>
  void forEachValue(const Slice& key, Procedure process) const {
//    if (auto list = getList(key)) {
//      list->forEachValue(process, *store_);
//    }
  }

  template <typename BinaryProcedure>
  void forEachEntry(BinaryProcedure process) const {
    ReaderLockGuard<boost::shared_mutex> lock(mutex_);
//    store_->adviseAccessPattern(Store::AccessPattern::WILLNEED);
//    for (const auto& entry : map_) {
//      auto iter = entry.second->newIterator(*store_);
//      if (iter->hasNext()) {
//        process(entry.first, iter.get());
//      }
//    }
//    store_->adviseAccessPattern(Store::AccessPattern::NORMAL);
  }

  Stats getStats() const;
  // Returns various statistics about the partition.
  // The data is collected upon request and triggers a full partition scan.

//  uint32_t getBlockSize() const { return store_->options().block_size; }

  // ---------------------------------------------------------------------------
  // Static member functions
  // ---------------------------------------------------------------------------

  template <typename BinaryProcedure>
  static void forEachEntry(const std::string& prefix, BinaryProcedure process) {
//    const auto stats = Stats::readFromFile(getNameOfStatsFile(prefix));

//    Store::Options store_options;
//    store_options.readonly = true;
//    store_options.block_size = stats.block_size;
//    Store store(getNameOfStoreFile(prefix), store_options);
//    store.adviseAccessPattern(Store::AccessPattern::WILLNEED);

//    Bytes key;
//    const auto stream = mt::open(getNameOfMapFile(prefix), "r");
//    for (size_t i = 0; i != stats.num_keys_valid; i++) {
//      MT_ASSERT_TRUE(readBytesFromStream(stream.get(), &key));
//      const List list = List::readFromStream(stream.get());
//      const auto iter = list.newIterator(store);
//      process(key, iter.get());
//    }
  }

  static std::string getNameOfMapFile(const std::string& prefix);
  static std::string getNameOfStatsFile(const std::string& prefix);
  static std::string getNameOfStoreFile(const std::string& prefix);

 private:
  struct Equal {
    explicit Equal(const Slice& value) : value_(value) {}

    bool operator()(const Slice& other) const { return other == value_; }

   private:
    const Slice value_;
  };

  List* getList(const Slice& key) const {
    ReaderLockGuard<boost::shared_mutex> lock(mutex_);
    const auto iter = map_.find(key);
    return (iter != map_.end()) ? iter->second.get() : nullptr;
  }

  List* getListOrCreate(const Slice& key) {
    MT_REQUIRE_LE(key.size(), Limits::maxKeySize());
    WriterLockGuard<boost::shared_mutex> lock(mutex_);
    auto iter = map_.find(key);
    if (iter == map_.end()) {
      iter = map_.emplace(key.makeCopy([this](size_t size){
                     return arena_.allocate(size);
                   }), std::unique_ptr<List>(new List())).first;
    }
    return iter->second.get();
  }

  mutable boost::shared_mutex mutex_;
  std::unordered_map<Slice, std::unique_ptr<List>> map_;
  std::unique_ptr<Store> store_;
  std::string prefix_;
  Arena arena_;
  Stats stats_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_PARTITION_HPP_INCLUDED
