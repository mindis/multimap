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
#include "multimap/internal/Varint.hpp"

namespace multimap {
namespace internal {

// TODO May be buggy, if Varint::kMaxValueStoredIn32Bits is not initialized.
const std::uint32_t UintVector::kMaxValue = Varint::kMaxValueStoredIn32Bits;

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

UintVector UintVector::ReadFromStream(std::istream& stream) {
  UintVector ids;
  std::uint16_t ids_size;
  stream.read(reinterpret_cast<char*>(&ids_size), sizeof ids_size);
  ids.put_offset_ = ids_size;
  assert(stream.good());
  ids.data_.reset(new byte[ids.put_offset_]);
  stream.read(reinterpret_cast<char*>(ids.data_.get()), ids.put_offset_);
  assert(stream.good());
  ids.end_offset_ = ids.put_offset_;
  return ids;
}

void UintVector::WriteToStream(std::ostream& stream) const {
  assert(!empty());
  stream.write(reinterpret_cast<const char*>(&put_offset_), sizeof put_offset_);
  assert(stream.good());
  stream.write(reinterpret_cast<const char*>(data_.get()), put_offset_);
  assert(stream.good());
}

std::vector<std::uint32_t> UintVector::Unpack() const {
  std::vector<std::uint32_t> ids;
  if (!empty()) {
    std::uint32_t delta, id = 0;
    std::uint32_t get_offset = 0;
    const auto last_id_offset = put_offset_ - sizeof id;
    while (get_offset != last_id_offset) {
      get_offset += Varint::ReadUint32(data_.get() + get_offset, &delta);
      ids.push_back(id + delta);
      id = ids.back();
    }
  }
  return ids;
}

void UintVector::Add(std::uint32_t value) {
  assert(value <= kMaxValue);
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
