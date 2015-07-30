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

#include "multimap/internal/Block.hpp"
#include <cstring>
#include <array>

namespace multimap {
namespace internal {

Block::Block() : data_(nullptr), size_(0), offset_(0) {}

Block::Block(byte* data, std::uint32_t size)
    : data_(data), size_(size), offset_(0) {
  assert(data != nullptr);
}

double Block::load_factor() const {
  return (size_ != 0) ? (static_cast<double>(offset_) / size_) : 0;
}

std::uint32_t Block::max_value_size() const {
  const int max_size =
      (size_ > kSizeOfValueSizeField) ? (size_ - kSizeOfValueSizeField) : 0;
  return std::min(max_size, 0x7fff);
}

bool Block::TryAdd(const Bytes& value) {
  assert(data_);
  Check(value.size() <= max_value_size(),
        "Reject value of %d bytes because it exceeds the maximum value size "
        "of %d bytes. Consider to choose a larger block size and try again.",
        value.size(), max_value_size());
  const std::int16_t value_size = value.size();
  const auto num_bytes_required = sizeof value_size + value_size;
  if (num_bytes_required > num_bytes_free()) {
    return false;
  }
  // For little-endian only.
  byte* ptr = data_ + offset_;
  *(ptr++) = value_size >> 8;
  *(ptr++) = value_size;
  std::memcpy(ptr, value.data(), value_size);
  offset_ += sizeof value_size;
  offset_ += value_size;
  return true;
}

bool operator==(const Block& lhs, const Block& rhs) {
  if (lhs.data() == rhs.data()) return true;
  return (lhs.size() == rhs.size())
             ? std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0
             : false;
}

SuperBlock SuperBlock::ReadFromFd(int fd) {
  std::array<byte, kSerializedSize> buffer;
  System::Read(fd, buffer.data(), buffer.size());
  auto pos = buffer.data();
  SuperBlock block;
  std::memcpy(&block.major_version, pos, sizeof block.major_version);
  pos += sizeof block.major_version;
  std::memcpy(&block.minor_version, pos, sizeof block.minor_version);
  pos += sizeof block.minor_version;
  std::memcpy(&block.block_size, pos, sizeof block.block_size);
  return block;
}

// TODO Write all fields.
void SuperBlock::WriteToFd(int fd) const {
  std::array<byte, kSerializedSize> buffer;
  auto pos = buffer.data();
  std::memcpy(pos, &major_version, sizeof major_version);
  pos += sizeof major_version;
  std::memcpy(pos, &minor_version, sizeof minor_version);
  pos += sizeof minor_version;
  std::memcpy(pos, &block_size, sizeof block_size);
  pos += sizeof block_size;
  assert(buffer.data() - pos <= kSerializedSize);
  System::Write(fd, buffer.data(), buffer.size());
}

bool operator==(const SuperBlock& lhs, const SuperBlock& rhs) {
  return lhs.major_version == rhs.major_version &&
         lhs.minor_version == rhs.minor_version &&
         lhs.block_size == rhs.block_size;
}

}  // namespace internal
}  // namespace multimap
