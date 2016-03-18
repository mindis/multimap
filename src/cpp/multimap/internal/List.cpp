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

#include "multimap/internal/Varint.hpp"

namespace multimap {
namespace internal {

namespace {

class ExclusiveIterator : public Iterator {
 public:
  ExclusiveIterator(List* list, Store* store) {}

  size_t available() const override { return 0; }

  bool hasNext() const override { return false; }

  Slice next() override { return Slice(); }

  Slice peekNext() override { return Slice(); }

  void remove() {}
};

class SharedIterator : public Iterator {
 public:
  SharedIterator(const List& list, const Store& store) {}

  size_t available() const override { return 0; }

  bool hasNext() const override { return false; }

  Slice next() override { return Slice(); }

  Slice peekNext() override { return Slice(); }
};

}  // namespace

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
  size_t bytes_written = Varint::writeToBuffer(block_.current(), block_.end(),
                                               value.size(), removed);
  if (bytes_written == 0) {
    flushUnlocked(store);
    bytes_written = Varint::writeToBuffer(block_.current(), block_.end(),
                                          value.size(), removed);
    MT_ASSERT_NOT_ZERO(bytes_written);
  }
  block_.offset += bytes_written;

  // Write value's data into the block.
  bytes_written = 0;
  while (bytes_written != value.size()) {
    const size_t count =
        mt::min(value.size() - bytes_written, block_.end() - block_.current());
    if (count == 0) {
      flushUnlocked(store);
      continue;
    }
    MT_ASSERT_NOT_ZERO(count);
    std::memcpy(block_.current(), value.data() + bytes_written, count);
    bytes_written += count;
    block_.offset += count;
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
