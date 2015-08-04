// This file is part of the Multimap library.  http://multimap.io
//
// Copyright (C) 2015  Martin Trenkmann
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

#include <cassert>
#include <cstring>
#include "multimap/internal/System.hpp"

namespace multimap {
namespace internal {

UintVector::UintVector() : end_offset_(0), put_offset_(0) {}

UintVector::UintVector(const UintVector& other)
    : end_offset_(other.end_offset_), put_offset_(other.put_offset_) {
  if (other.data_) {
    data_.reset(new byte[end_offset_]);
    std::memcpy(data_.get(), other.data_.get(), end_offset_);
  }
}

UintVector& UintVector::operator=(const UintVector& other) {
  if (&other != this) {
    end_offset_ = other.end_offset_;
    put_offset_ = other.put_offset_;
    data_.reset(new byte[end_offset_]);
    std::memcpy(data_.get(), other.data_.get(), end_offset_);
  }
  return *this;
}

UintVector UintVector::ReadFromStream(std::FILE* stream) {
  std::uint16_t data_size;
  System::Read(stream, &data_size, sizeof data_size);
  std::unique_ptr<byte[]> data(new byte[data_size]);
  System::Read(stream, data.get(), data_size);

  UintVector vector;
  vector.data_ = std::move(data);
  vector.end_offset_ = data_size;
  vector.put_offset_ = data_size;
  return vector;
}

void UintVector::WriteToStream(std::FILE* stream) const {
  assert(!empty());
  System::Write(stream, &put_offset_, sizeof put_offset_);
  System::Write(stream, data_.get(), put_offset_);
}

std::vector<std::uint32_t> UintVector::Unpack() const {
  std::vector<std::uint32_t> values;
  if (!empty()) {
    std::uint32_t delta = 0;
    std::uint32_t value = 0;
    std::uint32_t get_offset = 0;
    const auto last_value_offset = put_offset_ - sizeof value;
    while (get_offset != last_value_offset) {
      get_offset += Varint::ReadUint32(data_.get() + get_offset, &delta);
      values.push_back(value + delta);
      value = values.back();
    }
  }
  return values;
}

void UintVector::Add(std::uint32_t value) {
  assert(value <= max_value());
  AllocateMoreIfFull();
  if (empty()) {
    put_offset_ += Varint::WriteUint32(value, data_.get());
    put_offset_ += WriteUint32(value, data_.get() + put_offset_);
  } else {
    std::uint32_t prev_value;
    put_offset_ -= sizeof prev_value;
    ReadUint32(data_.get() + put_offset_, &prev_value);
    assert(prev_value < value);
    const std::uint32_t delta = value - prev_value;
    put_offset_ += Varint::WriteUint32(delta, data_.get() + put_offset_);
    put_offset_ += WriteUint32(value, data_.get() + put_offset_);
  }
}

std::size_t UintVector::ReadUint32(const byte* source, std::uint32_t* target) {
  std::memcpy(target, source, sizeof *target);
  return sizeof *target;
}

std::size_t UintVector::WriteUint32(std::uint32_t source, byte* target) {
  std::memcpy(target, &source, sizeof source);
  return sizeof source;
}

void UintVector::AllocateMoreIfFull() {
  const auto required_size = sizeof(std::uint32_t) * 2;
  if (end_offset_ - put_offset_ < required_size) {
    const std::size_t new_end_offset = end_offset_ * 1.5;
    const auto new_size = std::max(new_end_offset, required_size);
    std::unique_ptr<byte[]> new_data(new byte[new_size]);
    std::memcpy(new_data.get(), data_.get(), put_offset_);
    data_ = std::move(new_data);
    end_offset_ = new_size;
  }
}

}  // namespace internal
}  // namespace multimap
