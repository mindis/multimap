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

#include "multimap/internal/SharedMutex.hpp"

#include <mutex>

namespace multimap {
namespace internal {

namespace {

std::mutex shared_mutex_allocation_mutex;

} // namespace

void SharedMutex::lock() {
  {
    std::lock_guard<std::mutex> lock(shared_mutex_allocation_mutex);
    if (!mutex_) {
      mutex_ = allocate();
    }
    mutex_->refcount++;
  }
  // shared_mutex_allocation_mutex is unlocked here in order to avoid
  // a deadlock, because the following mutex acquisition might block.
  mutex_->lock();
}

bool SharedMutex::try_lock() {
  std::lock_guard<std::mutex> lock(shared_mutex_allocation_mutex);
  if (!mutex_) {
    mutex_ = allocate();
  }
  const auto success = mutex_->try_lock();
  if (success) mutex_->refcount++;
  return success;
}

void SharedMutex::unlock() {
  std::lock_guard<std::mutex> lock(shared_mutex_allocation_mutex);
  MT_ASSERT_NOT_ZERO(mutex_->refcount);
  mutex_->unlock();
  mutex_->refcount--;
  if (mutex_->refcount == 0) {
    deallocate(std::move(mutex_));
  }
}

void SharedMutex::lock_shared() {
  {
    std::lock_guard<std::mutex> lock(shared_mutex_allocation_mutex);
    if (!mutex_) {
      mutex_ = allocate();
    }
    mutex_->refcount++;
  }
  // shared_mutex_allocation_mutex is unlocked here in order to avoid
  // a deadlock, because the following mutex acquisition might block.
  mutex_->lock_shared();
}

bool SharedMutex::try_lock_shared() {
  std::lock_guard<std::mutex> lock(shared_mutex_allocation_mutex);
  if (!mutex_) {
    mutex_ = allocate();
  }
  const auto success = mutex_->try_lock_shared();
  if (success) mutex_->refcount++;
  return success;
}

void SharedMutex::unlock_shared() {
  std::lock_guard<std::mutex> lock(shared_mutex_allocation_mutex);
  MT_ASSERT_NOT_ZERO(mutex_->refcount);
  mutex_->unlock_shared();
  mutex_->refcount--;
  if (mutex_->refcount == 0) {
    deallocate(std::move(mutex_));
  }
}

// Note that this function needs external locking.
std::unique_ptr<SharedMutex::RefCountedMutex> SharedMutex::allocate() {
  return std::unique_ptr<RefCountedMutex>(new RefCountedMutex());
}

// Note that this function needs external locking.
void SharedMutex::deallocate(std::unique_ptr<RefCountedMutex> mutex) {
  // Let it die.
}

}  // namespace internal
}  // namespace multimap
