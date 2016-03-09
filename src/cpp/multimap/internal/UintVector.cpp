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

#include "multimap/internal/Varint.hpp"

namespace multimap {
namespace internal {

namespace {

inline const byte* readUint32FromBuffer(const byte* buffer, uint32_t* value) {
  std::memcpy(value, buffer, sizeof *value);
  return buffer + sizeof *value;
}

inline byte* writeUint32ToBuffer(byte* buffer, uint32_t value) {
  std::memcpy(buffer, &value, sizeof value);
  return buffer + sizeof value;
}

}  // namespace

void UintVector::add(uint32_t value) {
  allocateMoreIfFull();
  if (empty()) {
    offset_ = Varint::writeToBuffer(pos(), end(), value) - beg();
  } else {
    uint32_t prev_value;
    offset_ -= sizeof prev_value;
    readUint32FromBuffer(pos(), &prev_value);
    MT_ASSERT_LT(prev_value, value);
    const uint32_t delta = value - prev_value;
    offset_ = Varint::writeToBuffer(pos(), end(), delta) - beg();
  }
  offset_ = writeUint32ToBuffer(pos(), value) - beg();
}

std::vector<uint32_t> UintVector::unpack() const {
  std::vector<uint32_t> values;
  if (!empty()) {
    uint32_t delta = 0;
    uint32_t value = 0;
    const byte* ptr = beg();
    const byte* end_of_deltas = pos() - sizeof value;
    while (ptr != end_of_deltas) {
      ptr = Varint::readFromBuffer(ptr, &delta);
      values.push_back(value + delta);
      value = values.back();
    }
  }
  return values;
}

UintVector UintVector::readFromStream(std::FILE* stream) {
  UintVector vector;
  mt::read(stream, &vector.offset_, sizeof vector.offset_);
  vector.data_.reset(new byte[vector.offset_]);
  mt::read(stream, vector.beg(), vector.offset_);
  vector.size_ = vector.offset_;
  return vector;
}

void UintVector::writeToStream(std::FILE* stream) const {
  mt::write(stream, &offset_, sizeof offset_);
  mt::write(stream, beg(), offset_);
}

void UintVector::allocateMoreIfFull() {
  const uint32_t required_size = sizeof(uint32_t) * 2;
  if (required_size > size_ - offset_) {
    const uint32_t new_end_offset = size_ * 1.5;
    const auto new_size = std::max(new_end_offset, required_size);
    std::unique_ptr<byte[]> new_data(new byte[new_size]);
    std::memcpy(new_data.get(), beg(), offset_);
    data_ = std::move(new_data);
    size_ = new_size;
  }
}

}  // namespace internal
}  // namespace multimap
