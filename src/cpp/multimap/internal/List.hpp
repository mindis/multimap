// This file is part of the Multimap library.  http://multimap.io
//
// Copyright (C) 2015  Martin Trenkmann
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

#include <functional>
#include <mutex>
#include <vector>
#include <boost/thread/shared_mutex.hpp>
#include "multimap/internal/Arena.hpp"
#include "multimap/internal/Store.hpp"
#include "multimap/internal/UintVector.hpp"
#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/Bytes.hpp"

namespace multimap {
namespace internal {

// -----------------------------------------------------------------------------
// class List

class List {
  // This class is designed for minimal memory footprint since
  // there will be an instance for each key put into Multimap.
  // The following patterns were applied:
  //
  //   * Dependency injection, e.g. for `List::add()`.
  //   * `List::mutex_` is allocated only on demand.

public:
  struct Limits {
    static std::size_t getMaxValueSize();
  };

  struct Head {
    std::uint32_t num_values_added = 0;
    std::uint32_t num_values_removed = 0;
    UintVector block_ids;

    std::size_t num_values_not_removed() const {
      MT_ASSERT_GE(num_values_added, num_values_removed);
      return num_values_added - num_values_removed;
    }

    static Head readFromFile(std::FILE* file);

    void writeToFile(std::FILE* file) const;
  };

  static_assert(mt::hasExpectedSize<Head>(20, 24),
                "class Head does not have expected size");

  template <bool IsMutable> class Iter {

    class Stream {
    public:
      static const std::size_t BLOCK_CACHE_SIZE = 1024;

      Stream() : store_(nullptr) {}

      MT_ENABLE_IF(IsMutable)
      Stream(List* list, Store* store)
          : block_ids_(list->head_.block_ids.unpack()),
            last_block_(list->block_.getView()),
            store_(store) {
        std::reverse(block_ids_.begin(), block_ids_.end());
      }

      MT_ENABLE_IF(!IsMutable)
      Stream(const List& list, const Store& store)
          : block_ids_(list.head_.block_ids.unpack()),
            last_block_(list.block_.getView()),
            store_(&store) {
        std::reverse(block_ids_.begin(), block_ids_.end());
      }

      ~Stream() { writeBackMutatedBlocks(); }

      Stream(Stream&&) = default;
      Stream& operator=(Stream&&) = default;

      void readSizeWithFlag(std::uint32_t* size, bool* flag) {
        std::size_t nbytes = 0;
        do {
          if (blocks_index_ < blocks_.size()) {
            auto& block = blocks_[blocks_index_];
            nbytes = block.readSizeWithFlag(size, flag);
            if (nbytes == 0) {
              ++blocks_index_;
            } else {
              size_with_flag_ptr_.index = blocks_index_;
              size_with_flag_ptr_.offset = block.offset() - nbytes;
            }

          } else if (block_ids_.empty()) {
            nbytes = last_block_.readSizeWithFlag(size, flag);
            MT_ASSERT_NOT_ZERO(nbytes);
            size_with_flag_ptr_.index = blocks_index_;
            size_with_flag_ptr_.offset = last_block_.offset() - nbytes;

          } else {
            loadNextBlocks(true);
          }
        } while (nbytes == 0);
      }

      void readData(char* target, std::uint32_t size) {
        std::size_t nbytes = 0;
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
            loadNextBlocks(false); // Keeps target of `size_with_flag_ptr_`.
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
        std::size_t index = 0;
        std::size_t offset = 0;
      } size_with_flag_ptr_;

      std::vector<std::uint32_t> block_ids_;
      // Elements are in reverse order to allow fast pop_front operation.

      std::vector<ExtendedReadWriteBlock> blocks_;
      std::size_t blocks_index_ = 0;

      ReadWriteBlock last_block_;
      // Contains a shallow copy of `list->block_` given in the constructor.

      typename std::conditional<IsMutable, Store, const Store>::type* store_;
      Arena arena_;
    };

  public:
    Iter() : list_(nullptr) {}

    MT_ENABLE_IF(IsMutable)
    Iter(List* list, Store* store) : list_(list), stream_(list, store) {
      stats_.available = list->head_.num_values_not_removed();
    }

    MT_ENABLE_IF(!IsMutable)
    Iter(const List& list, const Store& store)
        : list_(&list), stream_(list, store) {
      stats_.available = list.head_.num_values_not_removed();
    }

    Iter(Iter&&) = default;
    Iter& operator=(Iter&&) = default;

    Iter(const Iter&) = delete;
    Iter& operator=(const Iter&) = delete;

    std::size_t available() const { return stats_.available; }

    bool hasNext() { return available() != 0; }

    Bytes next() {
      MT_REQUIRE_TRUE(hasNext());
      const auto value = peekNext();
      stats_.load_next_value = true;
      --stats_.available;
      return value;
    }
    // Preconditions:
    //  * `hasNext()` yields `true`.

    Bytes peekNext() {
      if (stats_.load_next_value) {
        std::uint32_t value_size = 0;
        bool is_marked_as_removed = false;
        do {
          stream_.readSizeWithFlag(&value_size, &is_marked_as_removed);
          current_value_.resize(value_size);
          stream_.readData(current_value_.data(), value_size);
        } while (is_marked_as_removed);
        stats_.load_next_value = false;
      }
      return Bytes(current_value_.data(), current_value_.size());
    }
    // Preconditions:
    //  * `hasNext()` yields `true`.

    MT_ENABLE_IF(IsMutable) void remove() {
      stream_.overwriteLastExtractedFlag(true);
      ++list_->head_.num_values_removed;
    }
    // Preconditions:
    //  * `next()` must have been called.

  private:
    struct Stats {
      std::uint32_t available = 0;
      bool load_next_value = true;
    };

    typename std::conditional<IsMutable, List, const List>::type* list_;
    std::vector<char> current_value_;
    Stream stream_;
    Stats stats_;
  };

  typedef Iter<true> MutableIterator;
  typedef Iter<false> Iterator;

  List() = default;

  explicit List(const Head& head);

  List(List&&) = default;
  List& operator=(List&&) = default;

  void add(const Bytes& value, Store* store, Arena* arena);

  void flush(Store* store);

  const Head& head() const { return head_; }

  std::size_t size() const { return head_.num_values_not_removed(); }

  bool empty() const { return size() == 0; }

  std::size_t clear() {
    const auto num = size();
    head_ = Head();
    return num;
  }

  Iterator iterator(const Store& store) const { return Iterator(*this, store); }

  MutableIterator iterator(Store* store) {
    return MutableIterator(this, store);
  }

  // ---------------------------------------------------------------------------
  // Synchronization interface based on std::shared_mutex.

  void lock() const;

  bool try_lock() const;

  void unlock() const;

  void lock_shared() const;

  bool try_lock_shared() const;

  void unlock_shared() const;

  bool is_locked() const;

  struct MutexPoolConfig {
    static std::size_t getDefaultSize();
    static std::size_t getCurrentSize();
    static std::size_t getMaximumSize();
    static void setMaximumSize(std::size_t size);
    MutexPoolConfig() = delete;
  };

private:
  void createMutexUnlocked() const;
  void deleteMutexUnlocked() const;

  Head head_;
  ReadWriteBlock block_;

  struct RefCountedMutex : public boost::shared_mutex {
    std::uint32_t refcount = 0;
  };
  // `std::unique_ptr` requires type definition.
  mutable std::unique_ptr<RefCountedMutex> mutex_;
  friend class MutexPool;
};

static_assert(mt::hasExpectedSize<List>(40, 48),
              "class List does not have expected size");

inline bool operator==(const List::Head& lhs, const List::Head& rhs) {
  return (lhs.block_ids == rhs.block_ids) &&
         (lhs.num_values_added == rhs.num_values_added) &&
         (lhs.num_values_removed == rhs.num_values_removed);
}

inline bool operator!=(const List::Head& lhs, const List::Head& rhs) {
  return !(lhs == rhs);
}

template <> inline void List::Iter<true>::Stream::writeBackMutatedBlocks() {
  if (store_) {
    store_->replace(blocks_);
  }
}

// -----------------------------------------------------------------------------
// class SharedList

class SharedList {
public:
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

  SharedList(SharedList&& other)
      : list_(other.release()), store_(other.store_) {}

  ~SharedList() {
    if (list_) {
      list_->unlock_shared();
    }
  }

  SharedList& operator=(SharedList&& other) {
    if (list_) {
      list_->unlock_shared();
    }
    list_ = other.release();
    store_ = other.store_;
    return *this;
  }

  operator bool() const { return list_ != nullptr; }

  const List::Head& head() const { return list_->head(); }

  std::size_t size() const { return list_->size(); }

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

// -----------------------------------------------------------------------------
// class SharedListIterator

class SharedListIterator {
public:
  SharedListIterator() = default;

  SharedListIterator(SharedList&& list) : list_(std::move(list)) {
    if (list_) {
      iter_ = list_.list_->iterator(*list_.store_);
    }
  }

  SharedListIterator(SharedListIterator&&) = default;
  SharedListIterator& operator=(SharedListIterator&&) = default;

  SharedList release() {
    auto list = std::move(list_);
    iter_ = List::Iterator();
    list_ = SharedList();
    return list;
  }

  std::size_t available() const { return iter_.available(); }

  bool hasNext() { return iter_.hasNext(); }

  Bytes next() { return iter_.next(); }

  Bytes peekNext() { return iter_.peekNext(); }

private:
  List::Iterator iter_;
  SharedList list_;
};

// -----------------------------------------------------------------------------
// class UniqueList

class UniqueList {
public:
  UniqueList() = default;

  UniqueList(List* list, Store* store, Arena* arena)
      : list_(list), store_(store), arena_(arena) {
    list_->lock();
  }

  UniqueList(List* list, Store* store, Arena* arena, std::try_to_lock_t) {
    if (list->try_lock()) {
      list_ = list;
      store_ = store;
      arena_ = arena;
    }
  }

  UniqueList(UniqueList&& other)
      : list_(other.release()), store_(other.store_), arena_(other.arena_) {}

  ~UniqueList() {
    if (list_) {
      list_->unlock();
    }
  }

  UniqueList& operator=(UniqueList&& other) {
    if (list_) {
      list_->unlock();
    }
    list_ = other.release();
    store_ = other.store_;
    arena_ = other.arena_;
    return *this;
  }

  operator bool() const { return list_ != nullptr; }

  const List::Head& head() const { return list_->head(); }

  std::size_t size() const { return list_->size(); }

  bool empty() const { return list_->empty(); }

  void add(const Bytes& value) { list_->add(value, store_, arena_); }

  std::size_t clear() { return list_->clear(); }

private:
  List* release() {
    auto list = list_;
    list_ = nullptr;
    return list;
  }

  List* list_ = nullptr;
  Store* store_ = nullptr;
  Arena* arena_ = nullptr;

  friend class UniqueListIterator;
};

// -----------------------------------------------------------------------------
// class UniqueListIterator

class UniqueListIterator {
public:
  UniqueListIterator() = default;

  UniqueListIterator(UniqueList&& list) : list_(std::move(list)) {
    if (list_) {
      iter_ = list_.list_->iterator(list_.store_);
    }
  }

  UniqueListIterator(UniqueListIterator&&) = default;
  UniqueListIterator& operator=(UniqueListIterator&&) = default;

  UniqueList release() {
    auto list = std::move(list_);
    iter_ = List::MutableIterator();
    list_ = UniqueList();
    return list;
  }

  std::size_t available() const { return iter_.available(); }

  bool hasNext() { return iter_.hasNext(); }

  Bytes next() { return iter_.next(); }

  Bytes peekNext() { return iter_.peekNext(); }

  void remove() { iter_.remove(); }

private:
  List::MutableIterator iter_;
  UniqueList list_;
};

} // namespace internal
} // namespace multimap

#endif // MULTIMAP_INTERNAL_LIST_HPP_INCLUDED
