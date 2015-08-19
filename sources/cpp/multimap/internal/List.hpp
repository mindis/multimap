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

#include <cstdio>
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
    static Head ReadFromStream(std::FILE* stream);

    void WriteToStream(std::FILE* stream) const;

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
    Iter();

    Iter(const Head& head, const Block& block, const Callbacks& callbacks);

    Iter(Head* head, Block* block, const Callbacks& callbacks);

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
    struct Stats {
      std::uint32_t num_values_read_total = 0;
      std::uint32_t num_values_read_deleted = 0;
      std::uint32_t block_id_index = -1;
      bool block_has_changed = false;

      std::uint32_t num_values_read_not_deleted() const {
        return num_values_read_total - num_values_read_deleted;
      }
    };

    void Advance();

    void ReplaceCurrentBlock();

    // This method must only be called when the end of the current block_ is
    // reached. Otherwise values in the current block_ will be skipped and
    // the counters in stats_ will become invalid.
    bool RequestNextBlockAndInitIter();

    typename std::conditional<IsConst, const Head, Head>::type* head_;
    typename std::conditional<IsConst, const Block, Block>::type* block_;
    std::vector<std::uint32_t> block_ids_;
    Block::Iter<IsConst> block_iter_;
    Block requested_block_;
    Callbacks callbacks_;
    Arena arena_;
    Stats stats_;
  };

  typedef Iter<false> Iterator;
  typedef Iter<true> ConstIterator;

  List() = default;
  List(const Head& head);

  List(List&&) = delete;
  List& operator=(List&&) = delete;

  List(const List&) = delete;
  List& operator=(const List&) = delete;

  void Add(const Bytes& value, const Callbacks::AllocateBlock& allocate_block,
           const Callbacks::CommitBlock& commit_block);

  // Precondition (not tested internally): !locked()
  void Flush(const Callbacks::CommitBlock& commit_block);

  void Clear() { head_ = Head(); }

  const Head& head() const { return head_; }

  const Head& chead() const { return head_; }

  const Block& block() const { return block_; }

  const Block& cblock() const { return block_; }

  std::size_t size() const { return head_.num_values_not_deleted(); }

  bool empty() const { return size() == 0; }

  Iterator NewIterator(const Callbacks& callbacks);

  ConstIterator NewIterator(const Callbacks& callbacks) const;

  ConstIterator NewConstIterator(const Callbacks& callbacks) const;

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

template <bool IsConst>
List::Iter<IsConst>::Iter()
    : head_(nullptr), block_(nullptr) {}

template <>
List::Iter<true>::Iter(const Head& head, const Block& block,
                       const Callbacks& callbacks);

template <>
List::Iter<false>::Iter(Head* head, Block* block, const Callbacks& callbacks);

template <bool IsConst>
std::size_t List::Iter<IsConst>::NumValues() const {
  return head_ ? head_->num_values_not_deleted() : 0;
}

template <bool IsConst>
void List::Iter<IsConst>::SeekToFirst() {
  if (head_) {
    stats_ = Stats();
    if (RequestNextBlockAndInitIter()) {
      if (!HasValue()) {
        Next();
      }
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
      ++stats_.num_values_read_total;
      if (block_iter_.deleted()) {
        ++stats_.num_values_read_deleted;
      }
    }
    Advance();
  } while (block_iter_.has_value() && block_iter_.deleted());
}

template <bool IsConst>
void List::Iter<IsConst>::Advance() {
  block_iter_.advance();
  if (!block_iter_.has_value()) {
    RequestNextBlockAndInitIter();
  }
}

template <bool IsConst>
bool List::Iter<IsConst>::RequestNextBlockAndInitIter() {
  if (stats_.num_values_read_not_deleted() < head_->num_values_not_deleted()) {
    ++stats_.block_id_index;
    if (stats_.block_id_index < block_ids_.size()) {
      const auto block_id = block_ids_[stats_.block_id_index];
      callbacks_.request_block(block_id, &requested_block_, &arena_);
      block_iter_ =
          static_cast<
              typename std::conditional<IsConst, const Block, Block>::type*>(
              &requested_block_)->NewIterator();
    } else {
      block_iter_ = block_->NewIterator();
    }
    stats_.block_has_changed = false;
    return true;
  }
  return false;
}

template <>
void List::Iter<false>::Delete();

template <>
void List::Iter<false>::Advance();

template <>
void List::Iter<false>::ReplaceCurrentBlock();

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_LIST_HPP
