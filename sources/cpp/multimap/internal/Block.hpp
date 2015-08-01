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

#ifndef MULTIMAP_BLOCK_HPP
#define MULTIMAP_BLOCK_HPP

#include <functional>
#include <limits>
#include "multimap/Bytes.hpp"
#include "multimap/internal/Check.hpp"
#include "multimap/internal/System.hpp"

namespace multimap {
namespace internal {

//  1                                   block_size
// +-----------------------+-----+---------+------+
// |        value 1        | ... | value n | free |
// +-----------------------+-----+---------+------+
// |                       |
// |1       1 2   16 17    |
// +---------+------+------+
// | deleted | size | data |
// +---------+------+------+

// Note: When the raw data of a block obtained via data() and size() is written
// to disk the current put offset obtained via used() is not kept. Therefore,
// when the block is read back into memory its content can only be read via
// forward interation. In particular, because the former put offset is unknown
// then, it is not possible to add new values to the end of the block. Doing so
// would override the content at the beginning of the block. If a client needs
// to preserve the put offset, the result of used() must be written in addition
// to the actual block data.
class Block {
 public:
  static const std::size_t kSizeOfValueSizeField = sizeof(std::int16_t);

  template <bool IsConst>
  class Iter {
   public:
    typedef typename std::conditional<IsConst, const byte, byte>::type Byte;

    Iter();

    Iter(Byte* data, std::size_t size);

    bool has_value() const { return (offset_ < size_) && (value_size() != 0); }

    // Requires: has_value()
    Byte* value_data() const { return tellg() + kSizeOfValueSizeField; }

    // Requires: has_value()
    std::size_t value_size() const {
      std::int16_t size = 0;
      size += tellg()[0] << 8;
      size += tellg()[1];
      return size & 0x7fff;
    }

    Bytes value() const { return Bytes(value_data(), value_size()); }

    // Requires: has_value()
    bool deleted() const { return *tellg() & 0x80; }

    // EnableIf: not IsConst
    // Requires: has_value()
    void set_deleted();

    // Requires: has_value()
    void advance() { offset_ += kSizeOfValueSizeField + value_size(); }

   private:
    Byte* tellg() const { return data_ + offset_; }

    Byte* data_;
    std::uint32_t size_;
    std::uint32_t offset_;
  };

  typedef Iter<false> Iterator;
  typedef Iter<true> ConstIterator;

  Block();

  Block(byte* data, std::uint32_t size);

  byte* data() { return data_; }

  const byte* data() const { return data_; }

  std::uint32_t size() const { return size_; }

  std::uint32_t used() const { return offset_; }

  bool has_data() const { return data_ != nullptr; }

  double load_factor() const;

  std::uint32_t max_value_size() const;

  Iterator NewIterator() {
    // We do not use offset_ to indicate the end of data, because
    // this member is always zero when the block is read from disk.
    return has_data() ? Iterator(data_, size_) : Iterator();
  }

  ConstIterator NewIterator() const {
    // We do not use offset_ to indicate the end of data, because
    // this member is always zero when the block is read from disk.
    return has_data() ? ConstIterator(data_, size_) : ConstIterator();
  }

  // Writes a copy of value's data into the block.
  // Returns true if the value could be written successfully.
  // Returns false if the block has not enough room to store the data.
  bool TryAdd(const Bytes& value);

 private:
  std::size_t num_bytes_free() const { return size_ - offset_; }

  byte* data_;
  std::uint32_t size_;
  std::uint32_t offset_;
};

static_assert(HasExpectedSize<Block, 16, 16>::value,
              "class Block does not have expected size");

bool operator==(const Block& lhs, const Block& rhs);

template <bool IsConst>
Block::Iter<IsConst>::Iter()
    : data_(nullptr), size_(0), offset_(0) {}

template <bool IsConst>
Block::Iter<IsConst>::Iter(Byte* data, std::size_t size)
    : data_(data), size_(size), offset_(0) {
  assert(data != nullptr);
}

template <>
inline void Block::Iter<false>::set_deleted() {
  *tellg() |= 0x80;
}

//  1    SerializedSize
// +-------------------+
// |     meta data     |
// +-------------------+
class SuperBlock {
  // TODO Capture id of FarmHash.
 public:
  static const std::uint32_t kSerializedSize = 512;

  static SuperBlock ReadFromFd(int fd);

  void WriteToFd(int fd) const;

  std::uint32_t major_version = 0;
  std::uint32_t minor_version = 1;
  std::uint32_t block_size = 0;

  std::uint64_t num_values_total = 0;
  std::uint64_t num_values_deleted = 0;
  std::uint64_t num_blocks = 0;

  double load_factor_total = 0;
} __attribute__((packed));

static_assert(sizeof(SuperBlock) <= SuperBlock::kSerializedSize,
              "sizeof(SuperBlock) > SuperBlock::kSerializedSize");

bool operator==(const SuperBlock& lhs, const SuperBlock& rhs);

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_BLOCK_HPP
