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

#ifndef MULTIMAP_INTERNAL_LIST_H_
#define MULTIMAP_INTERNAL_LIST_H_

#include "multimap/internal/Locks.h"
#include "multimap/internal/SharedMutex.h"
#include "multimap/internal/Store.h"
#include "multimap/internal/UintVector.h"
#include "multimap/thirdparty/mt/assert.h"
#include "multimap/Arena.h"
#include "multimap/callables.h"
#include "multimap/Iterator.h"
#include "multimap/Slice.h"

namespace multimap {
namespace internal {

class List {
 public:
  struct Limits {
    static size_t maxValueSize();

    Limits() = delete;
  };

  struct Stats {
    uint32_t num_values_total = 0;
    uint32_t num_values_removed = 0;

    uint32_t num_values_valid() const {
      MT_REQUIRE_GE(num_values_total, num_values_removed);
      return num_values_total - num_values_removed;
    }
  };

  List() = default;

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

  bool tryGetStats(Stats* stats) const;

  Stats getStatsUnlocked() const;

  bool tryFlush(Store* store, Stats* stats = nullptr);

  void flushUnlocked(Store* store, Stats* stats = nullptr);

  bool empty() const;

  size_t clear();

  static List readFromStream(std::istream* stream);

  void writeToStream(std::ostream* stream) const;

 private:
  void appendUnlocked(const Slice& value, Store* store, Arena* arena);

  friend class ExclusiveIterator;
  friend class SharedIterator;

  mutable SharedMutex mutex_;
  UintVector block_ids_;
  Store::Block block_;
  Stats stats_;
};

MT_STATIC_ASSERT_SIZEOF(List, 36, 48);

// The following functions are only public for unit testing.

size_t readVarint32AndFlag(const byte* begin, const byte* end, uint32_t* value,
                           bool* flag);

size_t writeVarint32AndFlag(byte* begin, byte* end, uint32_t value, bool flag);

void setFlag(byte* buffer, bool flag);

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_LIST_H_
