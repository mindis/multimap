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

#include "multimap/internal/System.hpp"

namespace multimap {
namespace internal {

std::mutex List::mutex_allocation_protector;

List::Head List::Head::readFromFile(std::FILE* file) {
  Head head;
  System::read(file, &head.num_values_added, sizeof head.num_values_added);
  System::read(file, &head.num_values_removed, sizeof head.num_values_removed);
  head.block_ids = UintVector::readFromStream(file);
  return head;
}

void List::Head::writeToFile(std::FILE* file) const {
  System::write(file, &num_values_added, sizeof num_values_added);
  System::write(file, &num_values_removed, sizeof num_values_removed);
  block_ids.writeToStream(file);
}

List::List(const Head& head) : head_(head) {}

void List::add(const Bytes& value,
               const Callbacks::NewBlock& new_block_callback,
               const Callbacks::CommitBlock& commit_block_callback) {
  if (!block_.hasData()) {
    block_ = new_block_callback();
  }
  auto ok = block_.add(value);
  if (!ok) {
    flush(commit_block_callback);
    ok = block_.add(value);
    assert(ok);
  }
  ++head_.num_values_added;
}

void List::flush(const Callbacks::CommitBlock& commit_block_callback) {
  if (block_.empty()) return;
  head_.block_ids.add(commit_block_callback(block_));
  block_.clear();
}

List::MutableIterator List::iterator(
    const Callbacks::RequestBlocks& request_blocks_callback,
    const Callbacks::ReplaceBlocks& replace_blocks_callback) {
  return MutableIterator(&head_, &block_, request_blocks_callback,
                         replace_blocks_callback);
}

List::Iterator List::const_iterator(
    const Callbacks::RequestBlocks& request_blocks_callback) const {
  return Iterator(head_, block_, request_blocks_callback);
}

//void List::forEach(
//    const Callables::Procedure& action,
//    const Callbacks::RequestBlocks& request_blocks_callback) const {
//  auto iter = const_iterator(request_blocks_callback);
//  while (iter.hasNext()) {
//    action(iter.next());
//  }
//}

//void List::forEach(
//    const Callables::Predicate& action,
//    const Callbacks::RequestBlocks& request_blocks_callback) const {
//  auto iter = const_iterator(request_blocks_callback);
//  while (iter.hasNext()) {
//    if (!action(iter.next())) {
//      break;
//    }
//  }
//}

void List::lock() {
  {
    const std::lock_guard<std::mutex> lock(mutex_allocation_protector);
    createMutexUnlocked();
    ++mutex_use_count_;
  }
  mutex_->lock();
}

bool List::try_lock() {
  const std::lock_guard<std::mutex> lock(mutex_allocation_protector);
  createMutexUnlocked();
  const auto locked = mutex_->try_lock();
  if (locked) {
    ++mutex_use_count_;
  }
  return locked;
}

void List::unlock() {
  const std::lock_guard<std::mutex> lock(mutex_allocation_protector);
  assert(mutex_use_count_ > 0);
  mutex_->unlock();
  --mutex_use_count_;
  if (mutex_use_count_ == 0) {
    deleteMutexUnlocked();
  }
}

void List::lock_shared() {
  {
    const std::lock_guard<std::mutex> lock(mutex_allocation_protector);
    createMutexUnlocked();
    ++mutex_use_count_;
  }
  mutex_->lock_shared();
}

bool List::try_lock_shared(){
  const std::lock_guard<std::mutex> lock(mutex_allocation_protector);
  createMutexUnlocked();
  const auto locked = mutex_->try_lock_shared();
  if (locked) {
    ++mutex_use_count_;
  }
  return locked;
}

void List::unlock_shared() {
  const std::lock_guard<std::mutex> lock(mutex_allocation_protector);
  assert(mutex_use_count_ > 0);
  mutex_->unlock_shared();
  --mutex_use_count_;
  if (mutex_use_count_ == 0) {
    deleteMutexUnlocked();
  }
}

bool List::is_locked() const {
  const std::lock_guard<std::mutex> lock(mutex_allocation_protector);
  return mutex_use_count_ != 0;
}

void List::createMutexUnlocked() const {
  if (!mutex_) {
    mutex_.reset(new Mutex());
  }
}

void List::deleteMutexUnlocked() const {
  assert(mutex_use_count_ == 0);
  mutex_.reset();
}

}  // namespace internal
}  // namespace multimap
