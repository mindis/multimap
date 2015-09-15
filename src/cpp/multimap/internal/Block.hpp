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

#ifndef MULTIMAP_INCLUDE_INTERNAL_BLOCK_HPP
#define MULTIMAP_INCLUDE_INTERNAL_BLOCK_HPP

#include <cassert>
#include <functional>
#include "multimap/Bytes.hpp"
#include "multimap/internal/Check.hpp"

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
// to disk the current put offset obtained via used() is not stored. Therefore,
// when the block is read back into memory its content can only be read via
// forward interation. In particular, because the former put offset is unknown
// then, it is not possible to add new values to the end of the block. Doing so
// would override the content at the beginning of the block. If a client needs
// to preserve the put offset, the result of used() must be written in addition
// to the actual block data.
class Block {
 public:
  static const std::size_t SIZE_OF_VALUE_SIZE_FIELD = sizeof(std::int16_t);

  template <bool IsConst>
  class Iter {
   public:
    typedef typename std::conditional<IsConst, const char, char>::type Char;

    Iter() : data_(nullptr), size_(0), offset_(0) {}

    Iter(Char* data, std::size_t size) : data_(data), size_(size), offset_(0) {
      assert(data);
    }

    bool hasValue() const { return hasData() && (size() != 0); }

    Bytes getValue() const { return Bytes(data(), size()); }
    // @pre hasValue()

    bool isDeleted() const { return *tellGetAddress() & 0x80; }
    // @pre hasValue()

    void markAsDeleted() { *tellGetAddress() |= 0x80; }
    // @pre hasValue()

    void next() { offset_ += SIZE_OF_VALUE_SIZE_FIELD + size(); }
    // @pre hasValue()

    void nextNotDeleted() {
      do {
        next();
      } while (hasValue() && isDeleted());
    }
    // @pre hasValue()

    Char* data() const { return tellGetAddress() + SIZE_OF_VALUE_SIZE_FIELD; }
    // @pre hasValue()

    std::size_t size() const {
      std::int16_t size = 0;
      size += tellGetAddress()[0] << 8;
      size += tellGetAddress()[1];
      return size & 0x7fff;
    }
    // @pre hasValue()

   private:
    bool hasData() const {
      return (offset_ + SIZE_OF_VALUE_SIZE_FIELD) < size_;
    }

    Char* tellGetAddress() const { return data_ + offset_; }

    Char* data_;
    std::uint32_t size_;
    std::uint32_t offset_;
  };

  typedef Iter<false> Iterator;
  typedef Iter<true> ConstIterator;

  Block();

  Block(void* data, std::uint32_t size);

  char* data() { return data_; }

  const char* data() const { return data_; }

  std::uint32_t size() const { return size_; }

  std::uint32_t position() const { return position_; }

  bool hasData() const { return data_ != nullptr; }

  void setData(void* data, std::uint32_t size) {
    assert(data != nullptr);
    assert(size != 0);
    data_ = static_cast<char*>(data);
    size_ = size;
    position_ = 0;
    std::memset(data_, 0, size_);
  }

  void clear() {
    if (hasData()) {
      std::memset(data_, 0, size_);
      position_ = 0;
    }
  }

  bool empty() const { return position_ == 0; }

  double load_factor() const;

  std::uint32_t max_value_size() const;
  // Returns the maximum size of a value to be put when the block is empty.

  Iterator iterator() {
    // We do not use this->position_ to indicate the end of data, because
    // this member is always zero when the block is read from disk.
    return hasData() ? Iterator(data_, size_) : Iterator();
  }

  ConstIterator iterator() const {
    // We do not use this->position_ to indicate the end of data, because
    // this member is always zero when the block is read from disk.
    return hasData() ? ConstIterator(data_, size_) : ConstIterator();
  }

  bool add(const Bytes& value);
  // Writes a copy of value's data into the block.
  // Returns true if the value could be written successfully.
  // Returns false if the block has not enough room to store the data.

 private:
  std::size_t num_bytes_free() const { return size_ - position_; }

  char* data_;
  std::uint32_t size_;
  std::uint32_t position_;
};

static_assert(HasExpectedSize<Block, 12, 16>::value,
              "class Block does not have expected size");

bool operator==(const Block& lhs, const Block& rhs);

struct BlockWithId : Block {
  std::uint32_t id = -1;
  bool ignore = false;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INCLUDE_INTERNAL_BLOCK_HPP
