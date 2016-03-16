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

#include "multimap/internal/Arena.hpp"
#include "multimap/internal/Locks.hpp"
#include "multimap/internal/SharedMutex.hpp"
#include "multimap/internal/Store.hpp"

namespace multimap {
namespace internal {

class List {
 public:
  struct Limits {
    static uint32_t maxValueSize();
  };

  void append(const Slice& value, Store* store, Arena* arena) {
    WriterLockGuard<SharedMutex> lock(mutex_);
    if (block_.data == nullptr) {
      block_.size = store->getBlockSize();
      block_.data = arena->allocate(block_.size);
      std::memset(block_.data, 0, block_.size);
    }
    byte* pos = value.writeToBuffer(block_.current(), block_.end());
    if (pos == block_.current()) {
      block_ids_.add(store->put(block_));
      std::memset(block_.data, 0, block_.size);
      block_.offset = 0;
      pos = value.writeToBuffer(block_.current(), block_.end());
      MT_ASSERT_NE(pos, block_.current());
    }
    block_.offset = pos - block_.begin();
  }

private:
  mutable SharedMutex mutex_;
  uint32_t num_values_total_ = 0;
  uint32_t num_values_removed_ = 0;
  UintVector block_ids_;
  Store::Block block_;
};

static_assert(mt::hasExpectedSize<List>(36, 48),
              "class List does not have expected size");

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_LIST_HPP_INCLUDED
