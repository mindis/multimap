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

namespace multimap {
namespace internal {

uint32_t List::Limits::maxValueSize() {
  return Varint::Limits::MAX_N4_WITH_FLAG;
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

void List::appendUnlocked(const Range& value, Store* store, Arena* arena) {
  MT_REQUIRE_LE(value.size(), Limits::maxValueSize());
  MT_REQUIRE_LT(stats_.num_values_total, std::numeric_limits<uint32_t>::max());

  if (!block_.hasData()) {
    const auto block_size = store->getBlockSize();
    byte* data = arena->allocate(block_size);
    block_ = ReadWriteBlock(data, block_size);
  }

  // Write value's metadata.
  if (!block_.writeSizeWithFlag(value.size(), false)) {
    flushUnlocked(store);
    MT_ASSERT_TRUE(block_.writeSizeWithFlag(value.size(), false));
  }

  // Write value's data.
  auto nbytes = block_.writeData(value);
  if (nbytes < value.size()) {
    flushUnlocked(store);

    // The value does not fit into the local block as a whole.
    // Write the remaining bytes which cover entire blocks directly
    // to the block file.  Write the rest into the local block.

    std::vector<ExtendedReadOnlyBlock> blocks;
    const size_t block_size = block_.size();
    const byte* tail_data = value.begin() + nbytes;
    uint32_t remaining = value.size() - nbytes;
    while (remaining >= block_size) {
      blocks.emplace_back(tail_data, block_size);
      tail_data += block_size;
      remaining -= block_size;
    }
    store->put(blocks.begin(), blocks.end());
    for (const auto& block : blocks) {
      block_ids_.add(block.id);
    }
    if (remaining != 0) {
      nbytes = block_.writeData(Range(tail_data, remaining));
      MT_ASSERT_EQ(nbytes, remaining);
    }
  }
  stats_.num_values_total++;
}

}  // namespace internal
}  // namespace multimap
