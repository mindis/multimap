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

/*
class MutexPool {
  // This singleton is not thread-safe by design and needs external locking.

 public:
  typedef List::RefCountedMutex Mutex;

  static MutexPool& instance() {
    static MutexPool instance;
    return instance;
  }

  std::unique_ptr<Mutex> getMutex() {
    std::unique_ptr<Mutex> result;
    if (!mutexes_.empty()) {
      result = std::move(mutexes_.back());
      mutexes_.pop_back();
    }
    return result;
  }

  void putMutex(std::unique_ptr<Mutex>&& mutex) {
    if (mutex && mutexes_.size() < maximum_size_) {
      mutexes_.push_back(std::move(mutex));
    }
  }

  uint32_t getDefaultSize() { return 1024; }

  uint32_t getCurrentSize() { return mutexes_.size(); }

  uint32_t getMaximumSize() { return maximum_size_; }

  void setMaximumSize(uint32_t size) {
    maximum_size_ = size;
    if (maximum_size_ < mutexes_.size()) {
      mutexes_.resize(size);
    } else {
      while (mutexes_.size() < maximum_size_) {
        mutexes_.emplace_back(new Mutex());
      }
    }
  }

 private:
  MutexPool() { setMaximumSize(getDefaultSize()); }

  std::vector<std::unique_ptr<Mutex> > mutexes_;
  uint32_t maximum_size_;
};
*/

/*
void List::deleteMutexUnlocked() const {
  MT_REQUIRE_ZERO(mutex_->refcount);
  MutexPool::instance().putMutex(std::move(mutex_));
  mutex_.reset();
}

uint32_t List::MutexPoolConfig::getCurrentSize() {
  return MutexPool::instance().getCurrentSize();
}

uint32_t List::MutexPoolConfig::getDefaultSize() {
  return MutexPool::instance().getDefaultSize();
}

uint32_t List::MutexPoolConfig::getMaximumSize() {
  return MutexPool::instance().getMaximumSize();
}

void List::MutexPoolConfig::setMaximumSize(uint32_t size) {
  return MutexPool::instance().setMaximumSize(size);
}
*/

}  // namespace

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

size_t SharedMutex::getCurrentPoolSize() {
  std::lock_guard<std::mutex> lock(shared_mutex_allocation_mutex);
  return Pool::instance().getCurrentSize();
}

size_t SharedMutex::getMaximumPoolSize() {
  std::lock_guard<std::mutex> lock(shared_mutex_allocation_mutex);
  return Pool::instance().getMaximumSize();
}

void SharedMutex::setMaximumPoolSize(size_t size) {
  std::lock_guard<std::mutex> lock(shared_mutex_allocation_mutex);
  Pool::instance().setMaximumSize(size);
}

SharedMutex::Pool& SharedMutex::Pool::instance() {
  static Pool instance;
  return instance;
}

// Note: This function needs external locking.
size_t SharedMutex::Pool::getCurrentSize() const { return mutexes_.size(); }

// Note: This function needs external locking.
size_t SharedMutex::Pool::getMaximumSize() const { return max_size_; }

// Note: This function needs external locking.
void SharedMutex::Pool::setMaximumSize(size_t size) { max_size_ = size; }

// Note: This function needs external locking.
void SharedMutex::Pool::push(std::unique_ptr<RefCountedMutex> mutex) {
  if (mutexes_.size() < max_size_) {
    mutexes_.push_back(std::move(mutex));
  }
}

// Note: This function needs external locking.
std::unique_ptr<SharedMutex::RefCountedMutex> SharedMutex::Pool::pop() {
  std::unique_ptr<RefCountedMutex> mutex;
  if (!mutexes_.empty()) {
    mutex = std::move(mutexes_.back());
    mutexes_.pop_back();
  }
  return mutex;
}

// Note: This function needs external locking.
std::unique_ptr<SharedMutex::RefCountedMutex> SharedMutex::allocate() {
  auto mutex = Pool::instance().pop();
  if (!mutex) mutex.reset(new RefCountedMutex());
  return mutex;
}

// Note: This function needs external locking.
void SharedMutex::deallocate(std::unique_ptr<RefCountedMutex> mutex) {
  Pool::instance().push(std::move(mutex));
}

}  // namespace internal
}  // namespace multimap
