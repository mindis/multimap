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

#include "multimap/internal/List.hpp"

#include "multimap/internal/BlockPool.hpp"
#include "multimap/internal/System.hpp"

namespace multimap {
namespace internal {

std::mutex List::mutex_allocation_protector;

List::Head List::Head::ReadFromFile(std::FILE* file) {
  Head head;
  System::Read(file, &head.num_values_total, sizeof head.num_values_total);
  System::Read(file, &head.num_values_deleted, sizeof head.num_values_deleted);
  head.block_ids = UintVector::ReadFromStream(file);
  return head;
}

void List::Head::WriteToFile(std::FILE* file) const {
  System::Write(file, &num_values_total, sizeof num_values_total);
  System::Write(file, &num_values_deleted, sizeof num_values_deleted);
  block_ids.WriteToStream(file);
}

template <>
List::Iter<true>::Iter(const Head& head, const Block& last_block,
                       const Callbacks::RequestBlocks& request_blocks_callback)
    : head_(&head),
      last_block_(&last_block),
      block_ids_(head.block_ids.Unpack()),
      request_blocks_callback_(request_blocks_callback) {
  assert(request_blocks_callback_);
}

template <>
List::Iter<false>::Iter(Head* head, Block* last_block,
                        const Callbacks::RequestBlocks& request_blocks_callback,
                        const Callbacks::ReplaceBlocks& replace_blocks_callback)
    : head_(head),
      last_block_(last_block),
      block_ids_(head->block_ids.Unpack()),
      request_blocks_callback_(request_blocks_callback),
      replace_blocks_callback_(replace_blocks_callback) {
  assert(last_block);
  assert(request_blocks_callback);
  assert(replace_blocks_callback);
}

template <>
void List::Iter<false>::Delete() {
  block_iter_.set_deleted();
  ++head_->num_values_deleted;
  if (state_.cached_blocks_index < cached_blocks_.size()) {
    cached_blocks_[state_.cached_blocks_index].ignore = false;
    // last_block_ is in-memory and therefore updated in-place.
  }
}

template <>
void List::Iter<false>::WriteBackMutatedBlocks() {
  if (replace_blocks_callback_) {
    replace_blocks_callback_(cached_blocks_);
    // last_block_ is in-memory and therefore updated in-place.
  }
}

List::List(const Head& head) : head_(head) {}

void List::Add(const Bytes& value,
               const Callbacks::NewBlock& allocate_block_callback,
               const Callbacks::CommitBlock& commit_block_callback) {
  if (!block_.has_data()) {
    block_ = allocate_block_callback();
  }
  auto ok = block_.Add(value);
  if (!ok) {
    head_.block_ids.Add(commit_block_callback(std::move(block_)));
    block_ = allocate_block_callback();
    ok = block_.Add(value);
    assert(ok);
  }
  ++head_.num_values_total;
}

void List::Flush(const Callbacks::CommitBlock& commit_block_callback) {
  if (block_.has_data()) {
    head_.block_ids.Add(commit_block_callback(std::move(block_)));
    block_.reset();
  }
}

List::Iterator List::NewIterator(
    const Callbacks::RequestBlocks& request_blocks_callback,
    const Callbacks::ReplaceBlocks& replace_blocks_callback) {
  return Iterator(&head_, &block_, request_blocks_callback,
                  replace_blocks_callback);
}

List::ConstIterator List::NewConstIterator(
    const Callbacks::RequestBlocks& request_blocks_callback) const {
  return ConstIterator(head_, block_, request_blocks_callback);
}

void List::ForEach(
    const Callables::Procedure& procedure,
    const Callbacks::RequestBlocks& request_blocks_callback) const {
  auto iter = NewConstIterator(request_blocks_callback);
  for (iter.SeekToFirst(); iter.HasValue(); iter.Next()) {
    procedure(iter.GetValue());
  }
}

void List::ForEach(
    const Callables::Predicate& predicate,
    const Callbacks::RequestBlocks& request_blocks_callback) const {
  auto iter = NewConstIterator(request_blocks_callback);
  for (iter.SeekToFirst(); iter.HasValue(); iter.Next()) {
    if (!predicate(iter.GetValue())) {
      break;
    }
  }
}

void List::LockShared() const {
  {
    const std::lock_guard<std::mutex> lock(mutex_allocation_protector);
    CreateMutexUnlocked();
    ++mutex_use_count_;
  }
  mutex_->lock_shared();
}

void List::LockUnique() const {
  {
    const std::lock_guard<std::mutex> lock(mutex_allocation_protector);
    CreateMutexUnlocked();
    ++mutex_use_count_;
  }
  mutex_->lock();
}

bool List::TryLockShared() const {
  const std::lock_guard<std::mutex> lock(mutex_allocation_protector);
  CreateMutexUnlocked();
  const auto locked = mutex_->try_lock_shared();
  if (locked) {
    ++mutex_use_count_;
  }
  return locked;
}

bool List::TryLockUnique() const {
  const std::lock_guard<std::mutex> lock(mutex_allocation_protector);
  CreateMutexUnlocked();
  const auto locked = mutex_->try_lock();
  if (locked) {
    ++mutex_use_count_;
  }
  return locked;
}

void List::UnlockShared() const {
  const std::lock_guard<std::mutex> lock(mutex_allocation_protector);
  assert(mutex_use_count_ > 0);
  mutex_->unlock_shared();
  --mutex_use_count_;
  if (mutex_use_count_ == 0) {
    DeleteMutexUnlocked();
  }
}

void List::UnlockUnique() const {
  const std::lock_guard<std::mutex> lock(mutex_allocation_protector);
  assert(mutex_use_count_ > 0);
  mutex_->unlock();
  --mutex_use_count_;
  if (mutex_use_count_ == 0) {
    DeleteMutexUnlocked();
  }
}

bool List::locked() const {
  const std::lock_guard<std::mutex> lock(mutex_allocation_protector);
  return mutex_use_count_ != 0;
}

void List::CreateMutexUnlocked() const {
  if (!mutex_) {
    mutex_.reset(new Mutex());
  }
}

void List::DeleteMutexUnlocked() const {
  assert(mutex_use_count_ == 0);
  mutex_.reset();
}

}  // namespace internal
}  // namespace multimap
