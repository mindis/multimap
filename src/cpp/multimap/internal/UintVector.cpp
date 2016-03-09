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

#include "multimap/internal/Uint32Vector.hpp"

#include "multimap/internal/Varint.hpp"

namespace multimap {
namespace internal {

namespace {

inline const byte* readUint32(const byte* buffer, uint32_t* value) {
  std::memcpy(value, buffer, sizeof *value);
  return buffer + sizeof *value;
}

inline byte* writeUint32(byte* buffer, uint32_t value) {
  std::memcpy(buffer, &value, sizeof value);
  return buffer + sizeof value;
}

}  // namespace

Uint32Vector Uint32Vector::readFromStream(std::FILE* stream) {
  Uint32Vector vector;
  mt::fread(stream, &vector.offset_, sizeof vector.offset_);
  vector.data_.reset(new uint8_t[vector.offset_]);
  mt::fread(stream, vector.data_.get(), vector.offset_);
  vector.size_ = vector.offset_;
  return vector;
}

void Uint32Vector::writeToStream(std::FILE* stream) const {
  mt::fwrite(stream, &offset_, sizeof offset_);
  mt::fwrite(stream, data_.get(), offset_);
}

std::vector<uint32_t> Uint32Vector::unpack() const {
  std::vector<uint32_t> values;
  if (!empty()) {
    uint32_t delta = 0;
    uint32_t value = 0;
    const byte* begin = data_.get();
    const byte* end = begin + offset_ - sizeof value;
    while (begin != end) {
      begin = Varint::readUint32FromBuffer(begin, &delta);
      values.push_back(value + delta);
      value = values.back();
    }
  }
  return values;
}

void Uint32Vector::add(uint32_t value) {
  allocateMoreIfFull();
  if (empty()) {
    byte* new_pos = Varint::writeUint32ToBuffer(pos(), end(), value);
    new_pos = writeUint32(new_pos, value);
    offset_ = new_pos - data_.get();
  } else {
    uint32_t prev_value;
    offset_ -= sizeof prev_value;
    readUint32(pos(), &prev_value);
    MT_ASSERT_LT(prev_value, value);
    const uint32_t delta = value - prev_value;
    byte* new_pos = Varint::writeUint32ToBuffer(pos(), end(), delta);
    new_pos = writeUint32(pos(), value);
    offset_ = new_pos - data_.get();
  }
}

void Uint32Vector::allocateMoreIfFull() {
  const uint32_t required_size = sizeof(uint32_t) * 2;
  if (required_size > size_ - offset_) {
    const uint32_t new_end_offset = size_ * 1.5;
    const auto new_size = std::max(new_end_offset, required_size);
    std::unique_ptr<byte[]> new_data(new byte[new_size]);
    std::memcpy(new_data.get(), data_.get(), offset_);
    data_ = std::move(new_data);
    size_ = new_size;
  }
}

}  // namespace internal
}  // namespace multimap
