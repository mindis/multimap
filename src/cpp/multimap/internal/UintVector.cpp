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

#include "multimap/internal/UintVector.hpp"

#include <cstring>
#include "multimap/internal/Varint.hpp"
#include "multimap/thirdparty/mt/assert.hpp"
#include "multimap/thirdparty/mt/common.hpp"
#include "multimap/thirdparty/mt/fileio.hpp"

namespace multimap {
namespace internal {

namespace {

void readUint32FromBuffer(const byte* begin, const byte* end, uint32_t* value) {
  MT_REQUIRE_GE(static_cast<size_t>(end - begin), sizeof *value);
  std::memcpy(value, begin, sizeof *value);
}

void writeUint32ToBuffer(byte* begin, byte* end, uint32_t value) {
  MT_REQUIRE_GE(static_cast<size_t>(end - begin), sizeof value);
  std::memcpy(begin, &value, sizeof value);
}

}  // namespace

void UintVector::add(uint32_t value) {
  allocateMoreIfFull();
  if (empty()) {
    offset_ += Varint::writeToBuffer(begin(), end(), value);
  } else {
    uint32_t absolute_value = 0;
    readUint32FromBuffer(cur(), end(), &absolute_value);
    MT_ASSERT_LT(absolute_value, value);
    const uint32_t delta = value - absolute_value;
    offset_ += Varint::writeToBuffer(cur(), end(), delta);
  }
  // The new offset points past the last delta encoded value
  // which is also right before the trailing absolute value.
  writeUint32ToBuffer(cur(), end(), value);
}

std::vector<uint32_t> UintVector::unpack() const {
  std::vector<uint32_t> values;
  if (!empty()) {
    uint32_t delta = 0;
    uint32_t value = 0;
    const byte* pos = begin();
    const byte* end = cur();
    while (pos != end) {
      pos += Varint::readFromBuffer(pos, end, &delta);
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
  const uint32_t nbytes_required = sizeof(uint32_t) * 2;
  // Required are at most 4 bytes for the next varint-encoded
  // delta plus exactly 4 bytes for the trailing absolute value.
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
