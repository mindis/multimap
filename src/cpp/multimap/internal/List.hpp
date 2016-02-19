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

#ifndef MULTIMAP_INTERNAL_LIST_HPP_INCLUDED
#define MULTIMAP_INTERNAL_LIST_HPP_INCLUDED

#include <cstdio>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include <boost/thread/shared_lock_guard.hpp>
#include "multimap/internal/Arena.hpp"
#include "multimap/internal/SharedMutex.hpp"
#include "multimap/internal/Store.hpp"
#include "multimap/internal/UintVector.hpp"
#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/Iterator.hpp"

namespace multimap {
namespace internal {

class List : public mt::Resource {
  // This class is designed for minimal memory footprint in order to allow many
  // simultaneous instances. The following patterns were applied:
  //
  //   * Dependency injection, e.g. for `List::append()`.
  //   * `List::mutex_` is allocated only on demand.

 public:
  struct Limits {
    static uint32_t maxValueSize();
  };

  struct Stats {
    uint32_t num_values_total = 0;
    uint32_t num_values_removed = 0;

    uint32_t num_values_valid() const {
      MT_ASSERT_GE(num_values_total, num_values_removed);
      return num_values_total - num_values_removed;
    }

    static Stats readFromStream(std::FILE* file);
    void writeToStream(std::FILE* file) const;
  };

  static_assert(mt::hasExpectedSize<Stats>(8, 8),
                "class List::Stats does not have expected size");

  List() = default;

  static std::unique_ptr<List> readFromStream(std::FILE* stream);
  static void readFromStream(std::FILE* stream, List* list);
  void writeToStream(std::FILE* stream) const;

  void append(const Bytes& value, Store* store, Arena* arena) {
    WriterLockGuard lock(mutex_);
    appendUnlocked(value, store, arena);
  }

  template <typename InputIter>
  void append(InputIter first, InputIter last, Store* store, Arena* arena) {
    WriterLockGuard lock(mutex_);
    while (first != last) {
      appendUnlocked(*first, store, arena);
      ++first;
    }
  }

  std::unique_ptr<Iterator> newIterator(const Store& store) const {
    return newSharedIterator(store);
  }

  template <typename Predicate>
  bool removeOne(Predicate predicate, Store* store) {
    auto iter = newUniqueIterator(store);
    while (iter->hasNext()) {
      if (predicate(iter->next())) {
        iter->remove();
        return true;
      }
    }
    return false;
  }

  template <typename Predicate>
  uint32_t removeAll(Predicate predicate, Store* store) {
    uint32_t num_removed = 0;
    auto iter = newUniqueIterator(store);
    while (iter->hasNext()) {
      if (predicate(iter->next())) {
        iter->remove();
        ++num_removed;
      }
    }
    return num_removed;
  }

  template <typename Function>
  bool replaceOne(Function map, Store* store, Arena* arena) {
    std::vector<std::string> replaced_values;
    auto iter = newUniqueIterator(store);
    while (iter->hasNext()) {
      auto replaced_value = map(iter->next());
      if (!replaced_value.empty()) {
        replaced_values.push_back(std::move(replaced_value));
        iter->remove();
        break;
      }
    }
    // `iter` keeps the list in locked state.
    for (const auto& value : replaced_values) {
      appendUnlocked(value, store, arena);
    }
    return replaced_values.size();
  }

  template <typename Function>
  uint32_t replaceAll(Function map, Store* store, Arena* arena) {
    std::vector<std::string> replaced_values;
    auto iter = newUniqueIterator(store);
    while (iter->hasNext()) {
      auto replaced_value = map(iter->next());
      if (!replaced_value.empty()) {
        replaced_values.push_back(std::move(replaced_value));
        iter->remove();
      }
    }
    // `iter` keeps the list in locked state.
    for (const auto& value : replaced_values) {
      appendUnlocked(value, store, arena);
    }
    return replaced_values.size();
  }

  Stats getStats() const {
    ReaderLockGuard lock(mutex_);
    return getStatsUnlocked();
  }

  bool tryGetStats(Stats* stats) const {
    ReaderLock lock(mutex_, boost::try_to_lock);
    return lock ? (*stats = getStatsUnlocked(), true) : false;
  }

  Stats getStatsUnlocked() const { return stats_; }

  void flush(Store* store, Stats* stats = nullptr) {
    WriterLockGuard lock(mutex_);
    flushUnlocked(store, stats);
  }

  bool tryFlush(Store* store, Stats* stats = nullptr) {
    WriterLock lock(mutex_, std::try_to_lock);
    return lock ? (flushUnlocked(store, stats), true) : false;
  }

  void flushUnlocked(Store* store, Stats* stats = nullptr) {
    if (block_.hasData()) {
      block_.fillUpWithZeros();
      block_ids_.add(store->put(block_));
      block_.rewind();
    }
    if (stats) *stats = stats_;
  }

  uint32_t clear() {
    WriterLockGuard lock(mutex_);
    const auto num_removed = stats_.num_values_valid();
    stats_.num_values_removed = stats_.num_values_total;
    block_ids_.clear();
    return num_removed;
  }

  uint32_t size() const {
    ReaderLockGuard lock(mutex_);
    return stats_.num_values_valid();
  }

  bool empty() const { return size() == 0; }

 private:
  typedef boost::shared_lock<SharedMutex> ReaderLock;
  typedef boost::shared_lock_guard<SharedMutex> ReaderLockGuard;
  typedef std::unique_lock<SharedMutex> WriterLock;
  typedef std::lock_guard<SharedMutex> WriterLockGuard;

  template <bool IsMutable>
  class Iter : public Iterator {
    class Stream : public mt::Resource {
     public:
      static const uint32_t BLOCK_CACHE_SIZE = 1024;

      MT_ENABLE_IF(IsMutable)
      Stream(List* list, Store* store)
          : block_ids_(list->block_ids_.unpack()),
            last_block_(list->block_.getView()),
            store_(store) {
        std::reverse(block_ids_.begin(), block_ids_.end());
      }

      MT_DISABLE_IF(IsMutable)
      Stream(const List& list, const Store& store)
          : block_ids_(list.block_ids_.unpack()),
            last_block_(list.block_.getView()),
            store_(&store) {
        std::reverse(block_ids_.begin(), block_ids_.end());
      }

      ~Stream() { writeBackMutatedBlocks(); }

      void readSizeWithFlag(uint32_t* size, bool* flag) {
        while (true) {
          if (blocks_index_ < blocks_.size()) {
            auto& block = blocks_[blocks_index_];
            const auto nbytes = block.readSizeWithFlag(size, flag);
            if (nbytes == 0 || *size == 0) {
              // End of block or end of data is reached, read next block.
              ++blocks_index_;
              continue;
            } else {
              size_with_flag_ptr_.index = blocks_index_;
              size_with_flag_ptr_.offset = block.offset() - nbytes;
              return;
            }
          }
          if (block_ids_.empty()) {
            const auto nbytes = last_block_.readSizeWithFlag(size, flag);
            MT_ASSERT_NOT_ZERO(nbytes);
            size_with_flag_ptr_.index = blocks_index_;
            size_with_flag_ptr_.offset = last_block_.offset() - nbytes;
            return;
          }
          loadNextBlocks(true);
        }
      }

      void readData(char* target, uint32_t size) {
        uint32_t nbytes = 0;
        do {
          if (blocks_index_ < blocks_.size()) {
            auto& block = blocks_[blocks_index_];
            nbytes = block.readData(target, size);
            if (nbytes < size) {
              ++blocks_index_;
            }
            target += nbytes;
            size -= nbytes;

          } else if (block_ids_.empty()) {
            nbytes = last_block_.readData(target, size);
            MT_ASSERT_EQ(nbytes, size);
            size -= nbytes;

          } else {
            loadNextBlocks(false);  // Keeps target of `size_with_flag_ptr_`.
          }
        } while (size > 0);
      }

      MT_ENABLE_IF(IsMutable)
      void overwriteLastExtractedFlag(bool value) {
        if (size_with_flag_ptr_.index < blocks_.size()) {
          auto& block = blocks_[size_with_flag_ptr_.index];
          block.writeFlagAt(value, size_with_flag_ptr_.offset);
          block.ignore = false;
          // Triggers that the block is written back to the
          // store when `writeBackMutatedBlocks()` is called.
        } else {
          last_block_.writeFlagAt(value, size_with_flag_ptr_.offset);
        }
      }

     private:
      void loadNextBlocks(bool replace_current_blocks) {
        const auto block_size = store_->getBlockSize();
        if (replace_current_blocks) {
          writeBackMutatedBlocks();
          blocks_.clear();
          arena_.deallocateAll();
          blocks_.reserve(BLOCK_CACHE_SIZE);
          while (blocks_.size() < BLOCK_CACHE_SIZE && !block_ids_.empty()) {
            char* block_data = arena_.allocate(block_size);
            blocks_.emplace_back(block_data, block_size, block_ids_.back());
            block_ids_.pop_back();
          }
          store_->get(blocks_);
          for (auto& block : blocks_) {
            block.ignore = true;
            // Triggers that the block is not written back to the
            // store when `writeBackMutatedBlocks()` is called.
          }
          blocks_index_ = 0;

        } else {
          std::vector<ExtendedReadWriteBlock> blocks;
          blocks.reserve(BLOCK_CACHE_SIZE);
          while (blocks.size() < BLOCK_CACHE_SIZE && !block_ids_.empty()) {
            char* block_data = arena_.allocate(block_size);
            blocks.emplace_back(block_data, block_size, block_ids_.back());
            block_ids_.pop_back();
          }
          store_->get(blocks);
          for (auto& block : blocks) {
            blocks_.push_back(block);
            blocks_.back().ignore = true;
            // Triggers that the block is not written back to the
            // store when `writeBackMutatedBlocks()` is called.
          }
        }
      }

      void writeBackMutatedBlocks() {
        // There is a specialization when IsMutable is true.
      }

      struct IntoBlockPointer {
        uint32_t index = 0;
        uint32_t offset = 0;
      } size_with_flag_ptr_;

      std::vector<uint32_t> block_ids_;
      // Elements are in reverse order to allow fast pop_front operation.

      std::vector<ExtendedReadWriteBlock> blocks_;
      uint32_t blocks_index_ = 0;

      ReadWriteBlock last_block_;
      // Contains a shallow copy of `list->block_` given in the constructor.

      typename std::conditional<IsMutable, Store, const Store>::type* store_;
      Arena arena_;
    };

   public:
    Iter() : list_(nullptr) {}

    MT_ENABLE_IF(IsMutable)
    Iter(List* list, Store* store)
        : list_(list), stream_(new Stream(list, store)) {
      list_->mutex_.lock();
      stats_.available = list->stats_.num_values_valid();
    }

    MT_DISABLE_IF(IsMutable)
    Iter(const List& list, const Store& store)
        : list_(&list), stream_(new Stream(list, store)) {
      list_->mutex_.lock_shared();
      stats_.available = list.stats_.num_values_valid();
    }

    ~Iter() override;

    uint32_t available() const override { return stats_.available; }

    bool hasNext() const override { return available() != 0; }

    Bytes next() override {
      MT_REQUIRE_TRUE(hasNext());
      const auto value = peekNext();
      stats_.load_next_value = true;
      --stats_.available;
      return value;
    }
    // Preconditions:
    //  * `hasNext()` yields `true`.

    Bytes peekNext() override {
      if (stats_.load_next_value) {
        uint32_t value_size = 0;
        bool is_marked_as_removed = false;
        do {
          stream_->readSizeWithFlag(&value_size, &is_marked_as_removed);
          value_.resize(value_size);
          stream_->readData(value_.data(), value_size);
        } while (is_marked_as_removed);
        stats_.load_next_value = false;
      }
      return Bytes(value_.data(), value_.size());
    }
    // Preconditions:
    //  * `hasNext()` yields `true`.

    MT_ENABLE_IF(IsMutable) void remove() {
      stream_->overwriteLastExtractedFlag(true);
      ++list_->stats_.num_values_removed;
    }
    // Preconditions:
    //  * `next()` must have been called.

   private:
    struct Stats {
      uint32_t available = 0;
      bool load_next_value = true;
    };

    typename std::conditional<IsMutable, List, const List>::type* list_;
    std::unique_ptr<Stream> stream_;  // Make stack object
    std::vector<char> value_;
    Stats stats_;
  };

  typedef Iter<true> UniqueIterator;
  typedef Iter<false> SharedIterator;

  std::unique_ptr<UniqueIterator> newUniqueIterator(Store* store) {
    return std::unique_ptr<UniqueIterator>(new UniqueIterator(this, store));
  }

  std::unique_ptr<SharedIterator> newSharedIterator(const Store& store) const {
    return std::unique_ptr<SharedIterator>(new SharedIterator(*this, store));
  }

  // ---------------------------------------------------------------------------
  // Not thread-safe interface

  void appendUnlocked(const Bytes& value, Store* store, Arena* arena);

  /*
  struct MutexPoolConfig {
    static uint32_t getDefaultSize();
    static uint32_t getCurrentSize();
    static uint32_t getMaximumSize();
    static void setMaximumSize(uint32_t size);
    MutexPoolConfig() = delete;
  };
  */

  Stats stats_;
  UintVector block_ids_;
  ReadWriteBlock block_;
  mutable SharedMutex mutex_;
};

static_assert(mt::hasExpectedSize<List>(36, 48),
              "class List does not have expected size");

template <>
inline void List::Iter<true>::Stream::writeBackMutatedBlocks() {
  if (store_) {
    store_->replace(blocks_);
  }
}

template <>
inline List::Iter<true>::~Iter() {
  list_->mutex_.unlock();
}

template <>
inline List::Iter<false>::~Iter() {
  list_->mutex_.unlock_shared();
}

/*
class SharedList : public mt::Resource {
 public:
  typedef List::Iterator Iterator;

  SharedList() = default;

  SharedList(const List& list, const Store& store)
      : list_(&list), store_(&store) {
    list_->lock_shared();
  }

  SharedList(const List& list, const Store& store, std::try_to_lock_t) {
    if (list.try_lock_shared()) {
      list_ = &list;
      store_ = &store;
    }
  }

  ~SharedList() {
    if (list_) {
      list_->unlock_shared();
    }
  }

  operator bool() const { return list_ != nullptr; }

  const List::Head& head() const { return list_->head(); }

  Iterator iterator() const {
    return list_ ? list_->iterator(*store_) : Iterator();
  }

  uint32_t size() const { return list_->size(); }

  bool empty() const { return list_->empty(); }

 private:
  const List* release() {
    auto list = list_;
    list_ = nullptr;
    return list;
  }

  const List* list_ = nullptr;
  const Store* store_ = nullptr;

  friend class SharedListIterator;
};

class SharedListIterator : public Iterator {
 public:
  SharedListIterator() = default;

  SharedListIterator(const List& list, const Store& store)
    : shared_list_(list, store) {
    iter_ = shared_list_.iterator();
  }

  SharedListIterator(const List& list, const Store& store, std::try_to_lock_t)
    : shared_list_(list, store, std::try_to_lock) {
    if (shared_list_) {
      iter_ = shared_list_.iterator();
    }
  }

  virtual ~SharedListIterator() = default;

  virtual uint32_t available() const override { return iter_.available(); }

  virtual bool hasNext() const override { return iter_.hasNext(); }

  virtual Bytes next() override { return iter_.next(); }

  virtual Bytes peekNext() override { return iter_.peekNext(); }

 private:
  SharedList shared_list_;
  List::Iterator iter_;
};

class ExclusiveList : public mt::Resource {
 public:
  typedef List::MutableIterator Iterator;

  ExclusiveList() = default;

  ExclusiveList(List* list, Store* store, Arena* arena)
      : list_(list), store_(store), arena_(arena) {
    list_->lock();
  }

  ExclusiveList(List* list, Store* store, Arena* arena, std::try_to_lock_t) {
    if (list->try_lock()) {
      list_ = list;
      store_ = store;
      arena_ = arena;
    }
  }

  ~ExclusiveList() {
    if (list_) {
      list_->unlock();
    }
  }

  operator bool() const { return list_ != nullptr; }

  const List::Head& head() const { return list_->head(); }

  Iterator iterator() { return list_ ? list_->iterator(store_) : Iterator(); }

  uint32_t size() const { return list_->size(); }

  bool empty() const { return list_->empty(); }

  void add(const Bytes& value) { list_->add(value, store_, arena_); }

  void clear() { list_->clear(); }

 private:
  List* release() {
    auto list = list_;
    list_ = nullptr;
    return list;
  }

  List* list_ = nullptr;
  Store* store_ = nullptr;
  Arena* arena_ = nullptr;

  friend class ExclusiveListIterator;
};

class ExclusiveListIterator : public Iterator {
 public:
  ExclusiveListIterator() = default;

  ExclusiveListIterator(List* list, Store* store, Arena* arena)
    : exclusive_list_(list, store, arena) {
    iter_ = exclusive_list_.iterator();
  }

  ExclusiveListIterator(List* list, Store* store, Arena* arena,
std::try_to_lock_t)
    : exclusive_list_(list, store, arena, std::try_to_lock) {
    if (exclusive_list_) {
      iter_ = exclusive_list_.iterator();
    }
  }

  virtual ~ExclusiveListIterator() = default;

  virtual uint32_t available() const override { return iter_.available(); }

  virtual bool hasNext() const override { return iter_.hasNext(); }

  virtual Bytes next() override { return iter_.next(); }

  virtual Bytes peekNext() override { return iter_.peekNext(); }

  void remove() { iter_.remove(); }

 private:
  ExclusiveList exclusive_list_;
  List::MutableIterator iter_;
};
*/
}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_LIST_HPP_INCLUDED
