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
#include "multimap/internal/thirdparty/mt.hpp"
#include "multimap/internal/Arena.hpp"
#include "multimap/internal/Callbacks.hpp"
#include "multimap/internal/UintVector.hpp"

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
      assert(num_values_added >= num_values_removed);
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

    Iter(Head* head, Block* last_block,
         const Callbacks::RequestBlocks& request_blocks_callback,
         const Callbacks::ReplaceBlocks& replace_blocks_callback);

    ~Iter() { writeBackMutatedBlocks(); }

    Iter(Iter&&) = default;
    Iter& operator=(Iter&&) = default;

    void seekToFirst();

    bool hasValue() const;

    Bytes getValue() const;

    void next();

    void markAsDeleted();
    // Marks the current value as deleted.
    // Requires: hasValue()

    std::size_t num_values() const;

   private:
    struct State {
      std::uint32_t num_values_read = 0;
      std::uint32_t cached_blocks_index = -1;
      std::uint32_t cached_blocks_offset = 0;
    };

    Block::Iter<IsConst> getNextBlockIter();

    void writeBackMutatedBlocks();

    void readNextBlocks();

    typename std::conditional<IsConst, const Head, Head>::type* head_;
    typename std::conditional<IsConst, const Block, Block>::type* last_block_;

    Arena arena_;
    State state_;
    Block::Iter<IsConst> block_iter_;
    std::vector<std::uint32_t> block_ids_;
    std::vector<BlockWithId> cached_blocks_;
    Callbacks::RequestBlocks request_blocks_callback_;
    Callbacks::ReplaceBlocks replace_blocks_callback_;
  };

  typedef Iter<true> Iterator;
  typedef Iter<false> MutableIterator;

  typedef std::function<bool(const Bytes&)> BytesPredicate;
  typedef std::function<void(const Bytes&)> BytesProcedure;

  List() = default;
  List(const Head& head);

  List(List&&) = delete;
  List& operator=(List&&) = delete;

  List(const List&) = delete;
  List& operator=(const List&) = delete;

  void add(const Bytes& value,
           const Callbacks::NewBlock& allocate_block_callback,
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

  MutableIterator iterator(const Callbacks::RequestBlocks& request_blocks_callback,
                    const Callbacks::ReplaceBlocks& replace_blocks_callback);

  Iterator const_iterator(
      const Callbacks::RequestBlocks& request_blocks_callback) const;

  void forEach(const BytesProcedure& procedure,
               const Callbacks::RequestBlocks& request_blocks_callback) const;

  void forEach(const BytesPredicate& predicate,
               const Callbacks::RequestBlocks& request_blocks_callback) const;

  // Synchronization interface in tradition of std::mutex.

  void lockShared() const;

  void lockUnique() const;

  bool tryLockShared() const;

  bool tryLockUnique() const;

  void unlockShared() const;

  void unlockUnique() const;

  bool isLocked() const;

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
List::Iter<true>::Iter(const Head& head, const Block& last_block,
                       const Callbacks::RequestBlocks& request_blocks_callback);

template <>
List::Iter<false>::Iter(
    Head* head, Block* last_block,
    const Callbacks::RequestBlocks& request_blocks_callback,
    const Callbacks::ReplaceBlocks& replace_blocks_callback);

template <bool IsConst>
void List::Iter<IsConst>::seekToFirst() {
  if (head_) {
    arena_.reset();
    state_ = State();
    cached_blocks_.clear();
    block_iter_ = getNextBlockIter();
    if (block_iter_.hasValue() && block_iter_.isDeleted()) {
      next();
    }
  }
}

template <bool IsConst>
bool List::Iter<IsConst>::hasValue() const {
  return block_iter_.hasValue() && !block_iter_.isDeleted();
}

template <bool IsConst>
Bytes List::Iter<IsConst>::getValue() const {
  return Bytes(block_iter_.data(), block_iter_.size());
}

template <bool IsConst>
void List::Iter<IsConst>::next() {
  assert(block_iter_.hasValue());
  if (!block_iter_.isDeleted()) {
    ++state_.num_values_read;
  }
  while (true) {
    block_iter_.nextNotDeleted();
    if (!block_iter_.hasValue()) {
      block_iter_ = getNextBlockIter();
      if (block_iter_.hasValue() && block_iter_.isDeleted()) {
        continue;
      }
    }
    break;
  }
}

template <bool IsConst>
std::size_t List::Iter<IsConst>::num_values() const {
  return state_.head ? state_.heads->num_values_not_deleted() : 0;
}

template <bool IsConst>
Block::Iter<IsConst> List::Iter<IsConst>::getNextBlockIter() {
  ++state_.cached_blocks_index;
  if (state_.cached_blocks_index == cached_blocks_.size()) {
    writeBackMutatedBlocks();
    readNextBlocks();
    state_.cached_blocks_index = 0;
  }

  if (cached_blocks_.empty()) {
    auto abs_index = state_.cached_blocks_offset + state_.cached_blocks_index;
    return (abs_index == block_ids_.size()) ? last_block_->iterator()
                                            : Block::Iter<IsConst>();
  }
  auto& block = cached_blocks_[state_.cached_blocks_index];
  return static_cast<
             typename std::conditional<IsConst, const Block, Block>::type*>(
             &block)->iterator();
}

template <bool IsConst>
void List::Iter<IsConst>::writeBackMutatedBlocks() {
  // The default template does nothing.
}

template <bool IsConst>
void List::Iter<IsConst>::readNextBlocks() {
  static const std::size_t max_num_blocks_to_request = sysconf(_SC_IOV_MAX);
  state_.cached_blocks_offset += cached_blocks_.size();
  const auto num_blocks_to_request =
      std::min(max_num_blocks_to_request,
               block_ids_.size() - state_.cached_blocks_offset);
  cached_blocks_.resize(num_blocks_to_request);
  for (std::size_t i = 0; i != cached_blocks_.size(); ++i) {
    cached_blocks_[i].ignore = false;
    cached_blocks_[i].id = block_ids_[state_.cached_blocks_offset + i];
  }
  request_blocks_callback_(&cached_blocks_, &arena_);
  for (auto& block : cached_blocks_) {
    block.ignore = true;  // For subsequent replace_blocks_callback_.
  }
}

template <>
void List::Iter<false>::markAsDeleted();

template <>
void List::Iter<false>::writeBackMutatedBlocks();

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INCLUDE_INTERNAL_LIST_HPP
