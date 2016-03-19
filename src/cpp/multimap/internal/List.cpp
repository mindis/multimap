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

#include "multimap/internal/List.hpp"

#include <sys/mman.h>
#include <algorithm>
#include "multimap/internal/Varint.hpp"

namespace multimap {
namespace internal {

namespace {

class Stream {
 public:
  Stream() = default;

  Stream(const Store::Blocks& blocks, const Store::Block& tail)
      : blocks_(blocks), tail_(tail) {
    for (const Store::Block& block : blocks_) {
      int result = ::posix_madvise(block.data, block.size, POSIX_MADV_WILLNEED);
      mt::Check::isZero(result, "posix_madvise() failed");
    }
    std::reverse(blocks_.begin(), blocks_.end());
  }

  Slice next() {
    if (block_.empty()) {
      block_ = fetchNextBlock();
    }

    Slice value;
    size_t nbytes = 0;
    uint32_t size = 0;
    bool removed = false;
    do {
      // Read value's size and removed-flag.
      last_value_begin_ = block_.current();
      nbytes = Varint::readFromBuffer(block_.current(), &size, &removed);
      if (size == 0) {
        block_ = fetchNextBlock();
        last_value_begin_ = block_.current();
        nbytes = Varint::readFromBuffer(block_.current(), &size, &removed);
        MT_ASSERT_NOT_ZERO(size);
      }
      block_.offset += nbytes;

      // Read value's data.
      if (size <= (block_.size - block_.offset)) {
        value = Slice(block_.current(), size);
        block_.offset += size;
      } else {
        nbytes = 0;
        large_value_.resize(size);
        while (nbytes != size) {
          const size_t count =
              mt::min(size - nbytes, block_.size - block_.offset);
          if (count == 0) {
            block_ = fetchNextBlock();
            continue;
          }
          MT_ASSERT_NOT_ZERO(count);
          if (!removed) {
            std::memcpy(large_value_.data() + nbytes, block_.current(), count);
          }
          block_.offset += count;
          nbytes += count;
        }
        value = Slice(large_value_);
      }
    } while (removed);
    return value;
  }

  void markLastExtractedValueAsRemoved() {
    MT_REQUIRE_NOT_NULL(last_value_begin_);
    Varint::setFlagInBuffer(last_value_begin_, true);
  }

 private:
  Store::Block fetchNextBlock() {
    Store::Block block;
    if (blocks_.empty()) {
      block = tail_;
      tail_.clear();
    } else {
      block = blocks_.back();
      blocks_.pop_back();
    }
    MT_ASSERT_FALSE(block.empty());
    return block;
  }

  byte* last_value_begin_ = nullptr;
  Store::Blocks blocks_;
  Store::Block block_;
  Store::Block tail_;
  Bytes large_value_;
};

}  // namespace

class ExclusiveIterator : public Iterator {
 public:
  ExclusiveIterator(List* list, Store* store)
      : list_(list), store_(store), lock_(list->mutex_) {
    stream_ = Stream(store_->get(list_->block_ids_.unpack()), list_->block_);
    available_ = list_->stats_.num_values_valid();
  }

  size_t available() const override { return available_; }

  bool hasNext() const override { return available_ != 0; }

  Slice next() override {
    Slice value = peekNext();
    value_.clear();
    available_--;
    return value;
  }

  Slice peekNext() override {
    MT_REQUIRE_TRUE(hasNext());
    if (value_.empty()) {
      value_ = stream_.next();
    }
    return value_;
  }

  void remove() { stream_.markLastExtractedValueAsRemoved(); }

 private:
  Slice value_;
  Stream stream_;
  size_t available_ = 0;
  List* list_ = nullptr;
  Store* store_ = nullptr;
  WriterLockGuard<SharedMutex> lock_;
};

class SharedIterator : public Iterator {
 public:
  SharedIterator(const List& list, const Store& store)
      : list_(&list), store_(&store), lock_(list.mutex_) {
    stream_ = Stream(store_->get(list_->block_ids_.unpack()), list_->block_);
    available_ = list_->stats_.num_values_valid();
  }

  size_t available() const override { return available_; }

  bool hasNext() const override { return available_ != 0; }

  Slice next() override {
    Slice value = peekNext();
    value_.clear();
    available_--;
    return value;
  }

  Slice peekNext() override {
    MT_REQUIRE_TRUE(hasNext());
    if (value_.empty()) {
      value_ = stream_.next();
    }
    return value_;
  }

 private:
  Slice value_;
  Stream stream_;
  size_t available_ = 0;
  const List* list_ = nullptr;
  const Store* store_ = nullptr;
  ReaderLockGuard<SharedMutex> lock_;
};

size_t List::Limits::maxValueSize() { return Varint::Limits::MAX_N4_WITH_FLAG; }

void List::append(const Slice& value, Store* store, Arena* arena) {
  WriterLockGuard<SharedMutex> lock(mutex_);
  appendUnlocked(value, store, arena);
}

std::unique_ptr<Iterator> List::newIterator(const Store& store) const {
  return Iterator::newEmptyInstance();
}

void List::forEachValue(Procedure process, const Store& store) const {
  SharedIterator iter(*this, store);
  while (iter.hasNext()) {
    process(iter.next());
  }
}

bool List::removeFirstMatch(Predicate predicate, Store* store) {
  ExclusiveIterator iter(this, store);
  while (iter.hasNext()) {
    if (predicate(iter.next())) {
      iter.remove();
      return true;
    }
  }
  return false;
}

size_t List::removeAllMatches(Predicate predicate, Store* store) {
  size_t num_removed = 0;
  ExclusiveIterator iter(this, store);
  while (iter.hasNext()) {
    if (predicate(iter.next())) {
      iter.remove();
      num_removed++;
    }
  }
  return num_removed;
}

bool List::replaceFirstMatch(Function map, Store* store, Arena* arena) {
  ExclusiveIterator iter(this, store);
  while (iter.hasNext()) {
    const Bytes new_value = map(iter.next());
    if (!new_value.empty()) {
      appendUnlocked(new_value, store, arena);
      // `iter` keeps the list in locked state.
      iter.remove();
      return true;
    }
  }
  return false;
}

size_t List::replaceAllMatches(Function map, Store* store, Arena* arena) {
  std::vector<Bytes> new_values;
  ExclusiveIterator iter(this, store);
  while (iter.hasNext()) {
    Bytes new_value = map(iter.next());
    if (!new_value.empty()) {
      new_values.push_back(std::move(new_value));
      iter.remove();
    }
  }
  for (const auto& value : new_values) {
    appendUnlocked(value, store, arena);
    // `iter` keeps the list in locked state.
  }
  return new_values.size();
}

size_t List::replaceAllMatches(Function2 map, Store* store, Arena* arena) {
  Arena new_values_arena;
  std::vector<Slice> new_values;
  ExclusiveIterator iter(this, store);
  while (iter.hasNext()) {
    Slice new_value = map(iter.next(), &new_values_arena);
    if (!new_value.empty()) {
      new_values.push_back(new_value);
      iter.remove();
    }
  }
  for (const auto& value : new_values) {
    appendUnlocked(value, store, arena);
    // `iter` keeps the list in locked state.
  }
  return new_values.size();
}

void List::appendUnlocked(const Slice& value, Store* store, Arena* arena) {
  MT_REQUIRE_LE(value.size(), Limits::maxValueSize());
  MT_REQUIRE_LT(stats_.num_values_total, std::numeric_limits<uint32_t>::max());

  if (block_.data == nullptr) {
    block_.size = store->getOptions().block_size;
    block_.data = arena->allocate(block_.size);
    std::memset(block_.data, 0, block_.size);
  }

  // Write value's size and removed-flag into the block.
  const bool removed = false;
  size_t nbytes = Varint::writeToBuffer(block_.current(), block_.end(),
                                        value.size(), removed);
  if (nbytes == 0) {
    flushUnlocked(store);
    nbytes = Varint::writeToBuffer(block_.current(), block_.end(), value.size(),
                                   removed);
    MT_ASSERT_NOT_ZERO(nbytes);
  }
  block_.offset += nbytes;

  // Write value's data into the block.
  nbytes = 0;
  while (nbytes != value.size()) {
    const size_t count =
        mt::min(value.size() - nbytes, block_.size - block_.offset);
    if (count == 0) {
      flushUnlocked(store);
      continue;
    }
    MT_ASSERT_NOT_ZERO(count);
    std::memcpy(block_.current(), value.data() + nbytes, count);
    block_.offset += count;
    nbytes += count;
  }
  stats_.num_values_total++;
}

bool List::tryGetStats(Stats* stats) const {
  ReaderLock<SharedMutex> lock(mutex_, TRY_TO_LOCK);
  return lock ? (*stats = stats_, true) : false;
}

bool List::tryFlush(Store* store, Stats* stats) {
  WriterLock<SharedMutex> lock(mutex_, TRY_TO_LOCK);
  return lock ? (flushUnlocked(store, stats), true) : false;
}

void List::flushUnlocked(Store* store, Stats* stats) {
  if (block_.offset != 0) {
    block_ids_.add(store->put(block_));
    std::memset(block_.data, 0, block_.size);
    block_.offset = 0;
  }
  if (stats) *stats = stats_;
}

bool List::empty() const {
  ReaderLockGuard<SharedMutex> lock(mutex_);
  return stats_.num_values_valid() == 0;
}

size_t List::clear() {
  WriterLockGuard<SharedMutex> lock(mutex_);
  block_.offset = 0;
  block_ids_ = UintVector();
  const size_t num_removed = stats_.num_values_valid();
  stats_.num_values_removed = stats_.num_values_total;
  return num_removed;
}

List List::readFromStream(std::FILE* stream) {
  List list;
  mt::read(stream, &list.stats_.num_values_total,
           sizeof list.stats_.num_values_total);
  mt::read(stream, &list.stats_.num_values_removed,
           sizeof list.stats_.num_values_removed);
  list.block_ids_ = UintVector::readFromStream(stream);
  return std::move(list);
}

void List::writeToStream(std::FILE* stream) const {
  ReaderLockGuard<SharedMutex> lock(mutex_);
  mt::write(stream, &stats_.num_values_total, sizeof stats_.num_values_total);
  mt::write(stream, &stats_.num_values_removed,
            sizeof stats_.num_values_removed);
  block_ids_.writeToStream(stream);
}

}  // namespace internal
}  // namespace multimap
