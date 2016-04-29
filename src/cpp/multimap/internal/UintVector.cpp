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

#include "multimap/internal/UintVector.h"

#include "multimap/thirdparty/mt/assert.h"
#include "multimap/thirdparty/mt/common.h"
#include "multimap/thirdparty/mt/fileio.h"
#include "multimap/thirdparty/mt/varint.h"

namespace multimap {
namespace internal {

namespace {

uint32_t readFixedInt32FromBuffer(const byte* begin, const byte* end) {
  uint32_t result = 0;
  MT_REQUIRE_GE(static_cast<size_t>(end - begin), sizeof result);
  std::memcpy(&result, begin, sizeof result);
  return result;
}

void writeFixedInt32ToBuffer(byte* begin, byte* end, uint32_t value) {
  MT_REQUIRE_GE(static_cast<size_t>(end - begin), sizeof value);
  std::memcpy(begin, &value, sizeof value);
}

}  // namespace

void UintVector::add(uint32_t value) {
  allocateMoreIfFull();
  if (empty()) {
    offset_ += mt::writeVarint32ToBuffer(value, current(), end());
  } else {
    const uint32_t absolute_value = readFixedInt32FromBuffer(current(), end());
    MT_ASSERT_LT(absolute_value, value);
    const uint32_t delta = value - absolute_value;
    offset_ += mt::writeVarint32ToBuffer(delta, current(), end());
  }
  // The new offset points past the last delta encoded value
  // which is also right before the trailing absolute value.
  writeFixedInt32ToBuffer(current(), end(), value);
}

std::vector<uint32_t> UintVector::unpack() const {
  std::vector<uint32_t> values;
  if (!empty()) {
    uint32_t delta = 0;
    uint32_t value = 0;
    const byte* pos = begin();
    const byte* end = current();
    while (pos != end) {
      pos += mt::readVarint32FromBuffer(pos, &delta);
      values.push_back(value + delta);
      value = values.back();
    }
  }
  return values;
}

UintVector UintVector::readFromStream(std::FILE* stream) {
  UintVector vector;
  mt::freadAll(stream, &vector.size_, sizeof vector.size_);
  vector.data_.reset(new byte[vector.size_]);
  mt::freadAll(stream, vector.data_.get(), vector.size_);
  vector.offset_ = vector.size_ - sizeof(uint32_t);
  return vector;
}

void UintVector::writeToStream(std::FILE* stream) const {
  const uint32_t offset = offset_ + sizeof(uint32_t);
  mt::fwriteAll(stream, &offset, sizeof offset);
  mt::fwriteAll(stream, data_.get(), offset);
}

void UintVector::allocateMoreIfFull() {
  const uint32_t nbytes_required = mt::MAX_VARINT32_BYTES + sizeof(uint32_t);
  if (nbytes_required > size_ - offset_) {
    const uint32_t new_size = mt::max(size_ * 2, nbytes_required);
    std::unique_ptr<byte[]> new_data(new byte[new_size]);
    std::memcpy(new_data.get(), data_.get(), size_);
    data_ = std::move(new_data);
    size_ = new_size;
  }
}

}  // namespace internal
}  // namespace multimap
