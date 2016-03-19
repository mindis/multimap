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

#ifndef MULTIMAP_INTERNAL_LIST_HPP_INCLUDED
#define MULTIMAP_INTERNAL_LIST_HPP_INCLUDED

#include "multimap/internal/Locks.hpp"
#include "multimap/internal/SharedMutex.hpp"
#include "multimap/internal/Store.hpp"
#include "multimap/internal/UintVector.hpp"
#include "multimap/Arena.hpp"
#include "multimap/callables.hpp"
#include "multimap/Iterator.hpp"
#include "multimap/Slice.hpp"

namespace multimap {
namespace internal {

class List {
 public:
  struct Limits {
    static size_t maxValueSize();
  };

  struct Stats {
    uint32_t num_values_total = 0;
    uint32_t num_values_removed = 0;

    uint32_t num_values_valid() const {
      MT_REQUIRE_GE(num_values_total, num_values_removed);
      return num_values_total - num_values_removed;
    }
  };

  void append(const Slice& value, Store* store, Arena* arena);

  template <typename InputIter>
  void append(InputIter begin, InputIter end, Store* store, Arena* arena) {
    WriterLockGuard<SharedMutex> lock(mutex_);
    while (begin != end) {
      appendUnlocked(*begin, store, arena);
      ++begin;
    }
  }

  std::unique_ptr<Iterator> newIterator(const Store& store) const;

  void forEachValue(Procedure process, const Store& store) const;

  bool removeFirstMatch(Predicate predicate, Store* store);

  size_t removeAllMatches(Predicate predicate, Store* store);

  bool replaceFirstMatch(Function map, Store* store, Arena* arena);

  size_t replaceAllMatches(Function map, Store* store, Arena* arena);

  size_t replaceAllMatches(Function2 map, Store* store, Arena* arena);

  bool tryGetStats(Stats* stats) const;

  bool tryFlush(Store* store, Stats* stats = nullptr);

  void flushUnlocked(Store* store, Stats* stats = nullptr);

  bool empty() const;

  size_t clear();

  static List readFromStream(std::FILE* stream);

  void writeToStream(std::FILE* stream) const;

 private:
  void appendUnlocked(const Slice& value, Store* store, Arena* arena);

  friend class ExclusiveIterator;
  friend class SharedIterator;

  mutable SharedMutex mutex_;
  UintVector block_ids_;
  Store::Block block_;
  Stats stats_;
};

static_assert(mt::hasExpectedSize<List>(36, 48),
              "class List does not have expected size");

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_LIST_HPP_INCLUDED
