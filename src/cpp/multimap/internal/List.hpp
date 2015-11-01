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

#ifndef MULTIMAP_INCLUDE_INTERNAL_LIST_HPP
#define MULTIMAP_INCLUDE_INTERNAL_LIST_HPP

#include <unistd.h>  // For sysconf
#include <cstdio>
#include <functional>
#include <mutex>
#include <boost/thread/shared_mutex.hpp>
#include "multimap/internal/Arena.hpp"
#include "multimap/internal/Callbacks.hpp"
#include "multimap/internal/UintVector.hpp"
#include "multimap/thirdparty/mt.hpp"

namespace multimap {
namespace internal {

class List {
  static std::mutex mutex_allocation_protector;

 public:
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

  template <bool IsConst>
  class Iter {
   public:
    Iter() : head_(nullptr), last_block_(nullptr) {}

    Iter(const Head& head, const Block& last_block,
         const Callbacks::RequestBlocks& request_blocks_callback);
    // Specialized only for Iter<true>.

    Iter(Head* head, Block* last_block,
         const Callbacks::RequestBlocks& request_blocks_callback,
         const Callbacks::ReplaceBlocks& replace_blocks_callback);
    // Specialized only for Iter<false>.

    ~Iter() {
      if (head_) {
        head_->num_values_removed += stats_.num_values_removed;
      }
    }

    Iter(Iter&&) = default;
    Iter& operator=(Iter&&) = default;

    std::size_t available() const {
      return head_ ? head_->num_values_not_removed() - stats_.num_values_read
                   : 0;
    }

    bool hasNext() {
      if (block_iter_.hasNext()) {
        return true;
      }
      while (hasNextBlock()) {
        block_iter_ = nextBlockIter();
        if (block_iter_.hasNext()) {
          return true;
        }
      }
      return false;
    }

    Bytes next() {
      const auto value = block_iter_.next();
      ++stats_.num_values_read;
      return value;
    }

    Bytes peekNext() const { return block_iter_.peekNext(); }

    void remove();
    // Specialized only for Iter<false>.

   private:
    bool hasNextBlock() const {
      return block_cache_.hasNext() || (last_block_ && !stats_.last_block_read);
    }

    Block::Iter<IsConst> nextBlockIter() {
      if (block_cache_.hasNext()) {
        return block_cache_.next();
      }
      MT_ASSERT_NOT_NULL(last_block_);
      MT_ASSERT_FALSE(stats_.last_block_read);
      stats_.last_block_read = true;
      return last_block_->iterator();
    }

    class BlockCache {
     public:
      BlockCache() = default;

      BlockCache(std::vector<std::uint32_t>&& block_ids,
                 const Callbacks::RequestBlocks& request_blocks_callback);
      // Specialized only for Iter<true>.

      BlockCache(std::vector<std::uint32_t>&& block_ids,
                 const Callbacks::RequestBlocks& request_blocks_callback,
                 const Callbacks::ReplaceBlocks& replace_blocks_callback);
      // Specialized only for Iter<false>.

      ~BlockCache();
      // Specialized for Iter<true> and Iter<false>.

      BlockCache(BlockCache&&) = default;
      BlockCache& operator=(BlockCache&&) = default;

      bool hasNext() const { return offset_ + index_ < block_ids_.size(); }

      Block::Iter<IsConst> next();
      // Specialized for Iter<true> and Iter<false>.

     private:
      void writeBackMutatedBlocks();
      // Specialized only for Iter<false>.

      void readNextBlocks() {
        if (request_blocks_callback_) {
          static const std::size_t max_blocks_to_request = sysconf(_SC_IOV_MAX);

          index_ = 0;
          offset_ += blocks_.size();
          const auto num_blocks_to_request =
              std::min(max_blocks_to_request, block_ids_.size() - offset_);
          blocks_.resize(num_blocks_to_request);
          for (std::size_t i = 0; i != blocks_.size(); ++i) {
            blocks_[i].ignore = false;
            blocks_[i].id = block_ids_[offset_ + i];
          }
          if (!blocks_.empty()) {
            request_blocks_callback_(&blocks_, &arena_);
            for (auto& block : blocks_) {
              block.ignore = true;  // For subsequent replace_blocks_callback_.
            }
          }
        }
      }

      Arena arena_;
      std::size_t index_ = 0;
      std::size_t offset_ = 0;
      std::vector<BlockWithId> blocks_;
      std::vector<std::uint32_t> block_ids_;
      Callbacks::RequestBlocks request_blocks_callback_;
      Callbacks::ReplaceBlocks replace_blocks_callback_;
    };

    struct Stats {
      bool last_block_read = false;
      std::size_t num_values_read = 0;
      std::size_t num_values_removed = 0;
    };

    typename std::conditional<IsConst, const Head, Head>::type* head_;
    typename std::conditional<IsConst, const Block, Block>::type* last_block_;

    Block::Iter<IsConst> block_iter_;
    BlockCache block_cache_;
    Stats stats_;
  };

  typedef Iter<true> Iterator;
  typedef Iter<false> MutableIterator;

  List() = default;
  List(const Head& head);

  List(List&&) = delete;
  List& operator=(List&&) = delete;

  List(const List&) = delete;
  List& operator=(const List&) = delete;

  void add(const Bytes& value, const Callbacks::NewBlock& new_block_callback,
           const Callbacks::CommitBlock& commit_block_callback);

  void flush(const Callbacks::CommitBlock& commit_block_callback);
  // Requires: not isLocked()

  void clear() { head_ = Head(); }

  const Head& head() const { return head_; }

  const Head& chead() const { return head_; }

  const Block& block() const { return block_; }

  const Block& cblock() const { return block_; }

  std::size_t size() const { return head_.num_values_not_removed(); }

  bool empty() const { return size() == 0; }

  // TODO Rename
  MutableIterator iterator(
      const Callbacks::RequestBlocks& request_blocks_callback,
      const Callbacks::ReplaceBlocks& replace_blocks_callback);

  // TODO Rename
  Iterator const_iterator(
      const Callbacks::RequestBlocks& request_blocks_callback) const;

//  void forEach(const Callables::Procedure& action,
//               const Callbacks::RequestBlocks& request_blocks_callback) const;

//  void forEach(const Callables::Predicate& action,
//               const Callbacks::RequestBlocks& request_blocks_callback) const;

  // Synchronization interface based on std::shared_mutex.

  void lock();

  bool try_lock();

  void unlock();

  void lock_shared();

  bool try_lock_shared();

  void unlock_shared();

  bool is_locked() const;

 private:
  void createMutexUnlocked() const;
  void deleteMutexUnlocked() const;

  Head head_;
  Block block_;

  typedef boost::shared_mutex Mutex;

  mutable std::unique_ptr<Mutex> mutex_;
  mutable std::uint32_t mutex_use_count_ = 0;
};

static_assert(mt::hasExpectedSize<List>(40, 56),
              "class List does not have expected size");

template <>
inline List::Iter<true>::BlockCache::BlockCache(
    std::vector<std::uint32_t>&& block_ids,
    const Callbacks::RequestBlocks& request_blocks_callback)
    : block_ids_(block_ids), request_blocks_callback_(request_blocks_callback) {
  MT_REQUIRE_TRUE(request_blocks_callback);
}

template <>
inline List::Iter<false>::BlockCache::BlockCache(
    std::vector<std::uint32_t>&& block_ids,
    const Callbacks::RequestBlocks& request_blocks_callback,
    const Callbacks::ReplaceBlocks& replace_blocks_callback)
    : block_ids_(block_ids),
      request_blocks_callback_(request_blocks_callback),
      replace_blocks_callback_(replace_blocks_callback) {
  MT_REQUIRE_TRUE(request_blocks_callback);
  MT_REQUIRE_TRUE(replace_blocks_callback);
}

template <>
inline void List::Iter<false>::BlockCache::writeBackMutatedBlocks() {
  // TODO Remove replace_blocks_callback_ check.
  if (replace_blocks_callback_ && !blocks_.empty()) {
    replace_blocks_callback_(blocks_);
  }
}
// Needs to be specialized before any usage.

template <>
inline List::Iter<true>::BlockCache::~BlockCache() {}

template <>
inline List::Iter<false>::BlockCache::~BlockCache() {
  writeBackMutatedBlocks();
}

template <>
inline Block::Iter<true> List::Iter<true>::BlockCache::next() {
  MT_REQUIRE_TRUE(hasNext());

  if (index_ == blocks_.size()) {
    readNextBlocks();
  }

  MT_ASSERT_LT(index_, blocks_.size());
  return blocks_[index_++].const_iterator();
}

template <>
inline Block::Iter<false> List::Iter<false>::BlockCache::next() {
  MT_REQUIRE_TRUE(hasNext());

  if (index_ == blocks_.size()) {
    writeBackMutatedBlocks();
    readNextBlocks();
  }

  MT_ASSERT_LT(index_, blocks_.size());
  BlockWithId* block = &blocks_[index_++];
  return block->iterator([block]() { block->ignore = false; });
}

template <>
inline List::Iter<true>::Iter(
    const Head& head, const Block& last_block,
    const Callbacks::RequestBlocks& request_blocks_callback)
    : head_(&head),
      last_block_(&last_block),
      block_cache_(head.block_ids.unpack(), request_blocks_callback) {}

template <>
inline List::Iter<false>::Iter(
    Head* head, Block* last_block,
    const Callbacks::RequestBlocks& request_blocks_callback,
    const Callbacks::ReplaceBlocks& replace_blocks_callback)
    : head_(head),
      last_block_(last_block),
      block_cache_(head->block_ids.unpack(), request_blocks_callback,
                   replace_blocks_callback) {
  MT_REQUIRE_NOT_NULL(last_block);
}

template <>
inline List::Iter<true>::~Iter() {}

template <>
inline List::Iter<false>::~Iter() {
  if (head_) {
    head_->num_values_removed += stats_.num_values_removed;
  }
}

template <>
inline void List::Iter<false>::remove() {
  block_iter_.markAsDeleted();
  ++stats_.num_values_removed;
}

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INCLUDE_INTERNAL_LIST_HPP
