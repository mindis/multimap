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

std::mutex List::dynamic_mutex_protector;

List::Head List::Head::ReadFromStream(std::FILE* fs) {
  Head head;
  System::Read(fs, &head.num_values_total, sizeof head.num_values_total);
  System::Read(fs, &head.num_values_deleted, sizeof head.num_values_deleted);
  head.block_ids = UintVector::ReadFromStream(fs);
  return head;
}

void List::Head::WriteToStream(std::FILE* fs) const {
  System::Write(fs, &num_values_total, sizeof num_values_total);
  System::Write(fs, &num_values_deleted, sizeof num_values_deleted);
  block_ids.WriteToStream(fs);
}

template <>
List::Iter<true>::Iter(const Head& head, const Block& block,
                       const Callbacks& callbacks)
    : head_(&head),
      block_(&block),
      block_ids_(head_->block_ids.Unpack()),
      callbacks_(callbacks) {
  assert(callbacks_.allocate_block);
  assert(callbacks_.deallocate_block);
  assert(callbacks_.request_block);
}

template <>
List::Iter<false>::Iter(Head* head, Block* block, const Callbacks& callbacks)
    : head_(head),
      block_(block),
      block_ids_(head_->block_ids.Unpack()),
      callbacks_(callbacks) {
  assert(head_);
  assert(block_);
  assert(callbacks_.allocate_block);
  assert(callbacks_.deallocate_block);
  assert(callbacks_.request_block);
  assert(callbacks_.replace_block);
}

template <>
void List::Iter<false>::Delete() {
  assert(HasValue());
  stats_.block_has_changed |= !block_iter_.deleted();
  block_iter_.set_deleted();
  ++head_->num_values_deleted;
}

template <>
void List::Iter<false>::Advance() {
  block_iter_.advance();
  if (!block_iter_.has_value()) {
    if (stats_.block_has_changed) {
      UpdateCurrentBlock();
    }
    RequestNextBlockAndInitIter();
  }
}

template <>
void List::Iter<false>::UpdateCurrentBlock() {
  if (stats_.block_id_index < block_ids_.size()) {
    const auto block_id = block_ids_[stats_.block_id_index];
    callbacks_.replace_block(block_id, requested_block_);
  }
  // block_ is in-memory and therefore updated in-place.
}

List::List(const Head& head) : head_(head) {}

void List::Add(const Bytes& value,
               const Callbacks::AllocateBlock& allocate_block,
               const Callbacks::CommitBlock& commit_block) {
  if (!block_.has_data()) {
    block_ = allocate_block();
  }
  auto ok = block_.TryAdd(value);
  if (!ok) {
    head_.block_ids.Add(commit_block(std::move(block_)));
    block_ = allocate_block();
    ok = block_.TryAdd(value);
    assert(ok);
  }
  ++head_.num_values_total;
}

void List::Flush(const Callbacks::CommitBlock& commit_block) {
  if (block_.has_data()) {
    head_.block_ids.Add(commit_block(std::move(block_)));
    block_ = Block();
  }
}

List::Iterator List::NewIterator(const Callbacks& callbacks) {
  return Iterator(&head_, &block_, callbacks);
}

List::ConstIterator List::NewIterator(const Callbacks& callbacks) const {
  return ConstIterator(head_, block_, callbacks);
}

List::ConstIterator List::NewConstIterator(const Callbacks& callbacks) const {
  return ConstIterator(head_, block_, callbacks);
}

List::Head List::Copy(const Head& head, const DataFile& from_data_file,
                      BlockPool* from_block_pool, DataFile* to_data_file,
                      BlockPool* to_block_pool,
                      const Callables::Compare& compare) {
  Callbacks iter_callbacks;
  iter_callbacks.allocate_block = [from_block_pool]() {
    const auto block = from_block_pool->Pop();
    assert(block.has_data());
    return block;
  };
  iter_callbacks.deallocate_block = [from_block_pool](Block&& block) {
    from_block_pool->Push(std::move(block));
  };
  iter_callbacks.request_block =
      [&from_data_file](std::uint32_t block_id,
                        Block* block) { from_data_file.Read(block_id, block); };

  Callbacks list_callbacks;
  list_callbacks.allocate_block = [to_block_pool]() {
    const auto block = to_block_pool->Pop();
    assert(block.has_data());
    return block;
  };
  list_callbacks.commit_block = [to_data_file](Block&& block) {
    return to_data_file->Append(std::move(block));
  };

  List new_list;
  auto iter = List(head).NewConstIterator(iter_callbacks);
  if (compare) {
    // https://bitbucket.org/mtrenkmann/multimap/issues/5
    std::vector<std::string> values;
    values.reserve(iter.NumValues());
    for (iter.SeekToFirst(); iter.HasValue(); iter.Next()) {
      values.push_back(iter.GetValue().ToString());
    }
    std::sort(values.begin(), values.end(), compare);
    for (const auto& value : values) {
      new_list.Add(value, list_callbacks.allocate_block,
                   list_callbacks.commit_block);
    }
  } else {
    for (iter.SeekToFirst(); iter.HasValue(); iter.Next()) {
      new_list.Add(iter.GetValue(), list_callbacks.allocate_block,
                   list_callbacks.commit_block);
    }
  }
  new_list.Flush(list_callbacks.commit_block);
  return new_list.head();
}

void List::LockShared() const {
  {
    const std::lock_guard<std::mutex> lock(dynamic_mutex_protector);
    CreateMutexUnlocked();
    ++mutex_use_count_;
  }
  mutex_->lock_shared();
}

void List::LockUnique() const {
  {
    const std::lock_guard<std::mutex> lock(dynamic_mutex_protector);
    CreateMutexUnlocked();
    ++mutex_use_count_;
  }
  mutex_->lock();
}

bool List::TryLockShared() const {
  const std::lock_guard<std::mutex> lock(dynamic_mutex_protector);
  CreateMutexUnlocked();
  const auto locked = mutex_->try_lock_shared();
  if (locked) {
    ++mutex_use_count_;
  }
  return locked;
}

bool List::TryLockUnique() const {
  const std::lock_guard<std::mutex> lock(dynamic_mutex_protector);
  CreateMutexUnlocked();
  const auto locked = mutex_->try_lock();
  if (locked) {
    ++mutex_use_count_;
  }
  return locked;
}

void List::UnlockShared() const {
  const std::lock_guard<std::mutex> lock(dynamic_mutex_protector);
  assert(mutex_use_count_ > 0);
  mutex_->unlock_shared();
  --mutex_use_count_;
  if (mutex_use_count_ == 0) {
    DeleteMutexUnlocked();
  }
}

void List::UnlockUnique() const {
  const std::lock_guard<std::mutex> lock(dynamic_mutex_protector);
  assert(mutex_use_count_ > 0);
  mutex_->unlock();
  --mutex_use_count_;
  if (mutex_use_count_ == 0) {
    DeleteMutexUnlocked();
  }
}

bool List::locked() const {
  const std::lock_guard<std::mutex> lock(dynamic_mutex_protector);
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
