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

#ifndef MULTIMAP_INTERNAL_LIST_HPP
#define MULTIMAP_INTERNAL_LIST_HPP

#include <unistd.h>  // For sysconf
#include <cstdio>
#include <functional>
#include <mutex>
#include <boost/thread/shared_mutex.hpp>
#include "multimap/Callables.hpp"
#include "multimap/internal/Arena.hpp"
#include "multimap/internal/BlockPool.hpp"
#include "multimap/internal/Callbacks.hpp"
#include "multimap/internal/DataFile.hpp"
#include "multimap/internal/UintVector.hpp"

namespace multimap {
namespace internal {

class List {
  static std::mutex dynamic_mutex_protector;

 public:
  struct Head {
    static Head ReadFromFile(std::FILE* file);

    void WriteToFile(std::FILE* file) const;

    std::size_t num_values_not_deleted() const {
      assert(num_values_total >= num_values_deleted);
      return num_values_total - num_values_deleted;
    }

    std::uint32_t num_values_total = 0;
    std::uint32_t num_values_deleted = 0;
    UintVector block_ids;
  };

  static_assert(HasExpectedSize<Head, 24, 24>::value,
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

    ~Iter() { ReplaceChangedBlocks(); }

    Iter(Iter&&) = default;
    Iter& operator=(Iter&&) = default;

    std::size_t NumValues() const;

    void SeekToFirst();

    bool HasValue() const;

    Bytes GetValue() const;

    void Next();

    // Marks the current value as deleted.
    // Requires: HasValue()
    void Delete();

   private:
    struct State {
      std::uint32_t cached_blocks_index = -1;
      std::uint32_t cached_blocks_offset = 0;
      std::uint32_t num_values_read_total = 0;
      std::uint32_t num_values_read_deleted = 0;

      std::uint32_t num_values_read_not_deleted() const {
        return num_values_read_total - num_values_read_deleted;
      }
    };

    void Advance();

    void InitBlockIterWithNextBlock();

    void ReplaceChangedBlocks();

    void RequestNextBlocks();

    typename std::conditional<IsConst, const Head, Head>::type* head_;
    typename std::conditional<IsConst, const Block, Block>::type* last_block_;

    Arena arena_;
    State state_;
    Block::Iter<IsConst> block_iter_;
    std::vector<std::uint32_t> block_ids_;
    std::vector<Callbacks::Job> cached_blocks_;
    Callbacks::RequestBlocks request_blocks_callback_;
    Callbacks::ReplaceBlocks replace_blocks_callback_;
  };

  typedef Iter<false> Iterator;
  typedef Iter<true> ConstIterator;

  List() = default;
  List(const Head& head);

  List(List&&) = delete;
  List& operator=(List&&) = delete;

  List(const List&) = delete;
  List& operator=(const List&) = delete;

  void Add(const Bytes& value,
           const Callbacks::AllocateBlock& allocate_block_callback,
           const Callbacks::CommitBlock& commit_block_callback);

  // Precondition (not tested internally): !locked()
  void Flush(const Callbacks::CommitBlock& commit_block_callback);

  void Clear() { head_ = Head(); }

  const Head& head() const { return head_; }

  const Head& chead() const { return head_; }

  const Block& block() const { return block_; }

  const Block& cblock() const { return block_; }

  std::size_t size() const { return head_.num_values_not_deleted(); }

  bool empty() const { return size() == 0; }

  Iterator NewIterator(const Callbacks::RequestBlocks& request_blocks_callback,
                       const Callbacks::ReplaceBlocks& replace_blocks_callback);

  ConstIterator NewIterator(
      const Callbacks::RequestBlocks& request_blocks_callback) const;

  ConstIterator NewConstIterator(
      const Callbacks::RequestBlocks& request_blocks_callback) const;

  static Head Copy(const Head& head, const DataFile& from_data_file,
                   BlockPool* from_block_pool, DataFile* to_data_file,
                   BlockPool* to_block_pool, const Callables::Compare& compare);

  // Synchronization interface in tradition of std::mutex.

  void LockShared() const;

  void LockUnique() const;

  bool TryLockShared() const;

  bool TryLockUnique() const;

  void UnlockShared() const;

  void UnlockUnique() const;

  bool locked() const;

 private:
  void CreateMutexUnlocked() const;
  void DeleteMutexUnlocked() const;

  Head head_;
  Block block_;

  typedef boost::shared_mutex Mutex;

  mutable std::unique_ptr<Mutex> mutex_;
  mutable std::uint32_t mutex_use_count_ = 0;
};

static_assert(HasExpectedSize<List, 56, 56>::value,
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
std::size_t List::Iter<IsConst>::NumValues() const {
  return state_.head ? state_.heads->num_values_not_deleted() : 0;
}

template <bool IsConst>
void List::Iter<IsConst>::SeekToFirst() {
  if (head_) {
    arena_.Reset();
    state_ = State();
    cached_blocks_.clear();
    InitBlockIterWithNextBlock();
    if (!HasValue()) {
      Next();
    }
  }
}

template <bool IsConst>
bool List::Iter<IsConst>::HasValue() const {
  return block_iter_.has_value() && !block_iter_.deleted();
}

template <bool IsConst>
Bytes List::Iter<IsConst>::GetValue() const {
  return Bytes(block_iter_.value_data(), block_iter_.value_size());
}

template <bool IsConst>
void List::Iter<IsConst>::Next() {
  do {
    if (block_iter_.has_value()) {
      ++state_.num_values_read_total;
      if (block_iter_.deleted()) {
        ++state_.num_values_read_deleted;
      }
    }
    Advance();
  } while (block_iter_.has_value() && block_iter_.deleted());
}

template <bool IsConst>
void List::Iter<IsConst>::Advance() {
  block_iter_.advance();
  if (!block_iter_.has_value()) {
    InitBlockIterWithNextBlock();
  }
}

template <bool IsConst>
void List::Iter<IsConst>::InitBlockIterWithNextBlock() {
  ++state_.cached_blocks_index;
  if (state_.cached_blocks_index == cached_blocks_.size()) {
    ReplaceChangedBlocks();
    RequestNextBlocks();
    state_.cached_blocks_index = 0;
  }

  if (cached_blocks_.empty()) {
    auto abs_index = state_.cached_blocks_offset + state_.cached_blocks_index;
    if (abs_index == block_ids_.size()) {
      block_iter_ = last_block_->NewIterator();
    }
  } else {
    auto& block = cached_blocks_[state_.cached_blocks_index].block;
    block_iter_ =
        static_cast<
            typename std::conditional<IsConst, const Block, Block>::type*>(
            &block)->NewIterator();
  }
}

template <bool IsConst>
void List::Iter<IsConst>::ReplaceChangedBlocks() {
  // The default template does nothing.
}

template <bool IsConst>
void List::Iter<IsConst>::RequestNextBlocks() {
  static const std::size_t max_num_blocks_to_request = sysconf(_SC_IOV_MAX);
  state_.cached_blocks_offset += cached_blocks_.size();
  const auto num_blocks_to_request =
      std::min(max_num_blocks_to_request,
               block_ids_.size() - state_.cached_blocks_offset);
  cached_blocks_.resize(num_blocks_to_request);
  for (std::size_t i = 0; i != cached_blocks_.size(); ++i) {
    cached_blocks_[i].ignore = false;
    cached_blocks_[i].block_id = block_ids_[state_.cached_blocks_offset + i];
  }
  request_blocks_callback_(&cached_blocks_, &arena_);
  for (auto& block : cached_blocks_) {
    block.ignore = true;  // For subsequent replace_blocks_callback_.
  }
}

template <>
void List::Iter<false>::Delete();

template <>
void List::Iter<false>::ReplaceChangedBlocks();

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_LIST_HPP
