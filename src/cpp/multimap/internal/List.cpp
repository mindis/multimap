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

namespace multimap {
namespace internal {

namespace {

std::mutex list_mutex_allocation_mutex;

}  // namespace

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

  std::size_t getDefaultSize() { return 1024; }

  std::size_t getCurrentSize() { return mutexes_.size(); }

  std::size_t getMaximumSize() { return maximum_size_; }

  void setMaximumSize(std::size_t size) {
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
  std::size_t maximum_size_;
};

std::size_t List::Limits::maxValueSize() {
  return Varint::Limits::MAX_N4_WITH_FLAG;
}

List::Head List::Head::readFromStream(std::FILE* stream) {
  Head head;
  mt::fread(stream, &head.num_values_total, sizeof head.num_values_total);
  mt::fread(stream, &head.num_values_removed, sizeof head.num_values_removed);
  head.block_ids = UintVector::readFromStream(stream);
  return head;
}

void List::Head::writeToStream(std::FILE* stream) const {
  mt::fwrite(stream, &num_values_total, sizeof num_values_total);
  mt::fwrite(stream, &num_values_removed, sizeof num_values_removed);
  block_ids.writeToStream(stream);
}

List::List(const Head& head) : head_(head) {}

void List::add(const Bytes& value, Store* store, Arena* arena) {
  MT_REQUIRE_LE(value.size(), Limits::maxValueSize());

  if (!block_.hasData()) {
    const auto block_size = store->getBlockSize();
    char* data = arena->allocate(block_size);
    block_ = ReadWriteBlock(data, block_size);
  }

  // Write value's metadata.
  auto nbytes = block_.writeSizeWithFlag(value.size(), false);
  if (nbytes == 0) {
    flush(store);
    nbytes = block_.writeSizeWithFlag(value.size(), false);
    MT_ASSERT_NOT_ZERO(nbytes);
  }

  // Write value's data.
  nbytes = block_.writeData(value.data(), value.size());
  if (nbytes < value.size()) {
    flush(store);

    // The value does not fit into the local block as a whole.
    // Write the remaining bytes which cover entire blocks directly
    // to the block file.  Write the rest into the local block.

    std::vector<ExtendedReadOnlyBlock> blocks;
    const auto block_size = block_.size();
    const char* tail_data = value.data() + nbytes;
    std::size_t remaining = value.size() - nbytes;
    while (remaining >= block_size) {
      blocks.emplace_back(tail_data, block_size);
      tail_data += block_size;
      remaining -= block_size;
    }
    if (!blocks.empty()) {
      store->put(blocks);
      for (const auto& block : blocks) {
        head_.block_ids.add(block.id);
      }
    }
    if (remaining != 0) {
      nbytes = block_.writeData(tail_data, remaining);
      MT_ASSERT_EQ(nbytes, remaining);
    }
  }

  ++head_.num_values_total;
}

void List::flush(Store* store) {
  if (block_.hasData()) {
    block_.fillUpWithZeros();
    head_.block_ids.add(store->put(block_));
    block_.rewind();
  }
}

void List::lock() const {
  {
    std::lock_guard<std::mutex> lock(list_mutex_allocation_mutex);
    createMutexUnlocked();
    ++mutex_->refcount;
  }
  // `list_mutex_allocation_mutex` is unlocked here to avoid a deadlock,
  // because the following mutex acquisition might block.
  mutex_->lock();
}

bool List::try_lock() const {
  std::lock_guard<std::mutex> lock(list_mutex_allocation_mutex);
  createMutexUnlocked();
  const auto locked = mutex_->try_lock();
  if (locked) {
    ++mutex_->refcount;
  }
  // We do not need to delete the mutex if `try_lock` fails,
  // because this means that `mutex_->refcount` is not zero.
  return locked;
}

void List::unlock() const {
  std::lock_guard<std::mutex> lock(list_mutex_allocation_mutex);
  MT_REQUIRE_NOT_ZERO(mutex_->refcount);
  mutex_->unlock();
  --mutex_->refcount;
  if (mutex_->refcount == 0) {
    deleteMutexUnlocked();
  }
}

void List::lock_shared() const {
  {
    std::lock_guard<std::mutex> lock(list_mutex_allocation_mutex);
    createMutexUnlocked();
    ++mutex_->refcount;
  }
  // `list_mutex_allocation_mutex` is unlocked here to avoid a deadlock,
  // because the following mutex acquisition might block.
  mutex_->lock_shared();
}

bool List::try_lock_shared() const {
  std::lock_guard<std::mutex> lock(list_mutex_allocation_mutex);
  createMutexUnlocked();
  const auto locked = mutex_->try_lock_shared();
  if (locked) {
    ++mutex_->refcount;
  }
  // We do not need to delete the mutex if `try_lock` fails,
  // because this means that `mutex_->refcount` is not zero.
  return locked;
}

void List::unlock_shared() const {
  std::lock_guard<std::mutex> lock(list_mutex_allocation_mutex);
  MT_REQUIRE_NOT_ZERO(mutex_->refcount);
  mutex_->unlock_shared();
  --mutex_->refcount;
  if (mutex_->refcount == 0) {
    deleteMutexUnlocked();
  }
}

bool List::is_locked() const {
  std::lock_guard<std::mutex> lock(list_mutex_allocation_mutex);
  return mutex_.get();
}

void List::createMutexUnlocked() const {
  if (!mutex_) {
    mutex_ = MutexPool::instance().getMutex();
    if (!mutex_) {
      mutex_.reset(new MutexPool::Mutex());
    }
  }
}

void List::deleteMutexUnlocked() const {
  MT_REQUIRE_ZERO(mutex_->refcount);
  MutexPool::instance().putMutex(std::move(mutex_));
  mutex_.reset();
}

std::size_t List::MutexPoolConfig::getCurrentSize() {
  return MutexPool::instance().getCurrentSize();
}

std::size_t List::MutexPoolConfig::getDefaultSize() {
  return MutexPool::instance().getDefaultSize();
}

std::size_t List::MutexPoolConfig::getMaximumSize() {
  return MutexPool::instance().getMaximumSize();
}

void List::MutexPoolConfig::setMaximumSize(std::size_t size) {
  return MutexPool::instance().setMaximumSize(size);
}

}  // namespace internal
}  // namespace multimap
