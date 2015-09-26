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
#include "multimap/internal/thirdparty/mt.hpp"

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

  static std::size_t max_value_size(std::size_t block_size) {
    MT_REQUIRE_GT(block_size, SIZE_OF_VALUE_SIZE_FIELD);
    return block_size - SIZE_OF_VALUE_SIZE_FIELD;
  }

  template <bool IsConst>
  class Iter {
   public:
    typedef std::function<void()> HasChanged;

    typedef typename std::conditional<IsConst, const char, char>::type Char;

    Iter() = default;

    Iter(Char* data, std::size_t size) : data_(data), data_size_(size) {
      MT_REQUIRE_NOT_NULL(data);
    }

    Iter(char* data, std::size_t size, HasChanged has_changed_callback);
    // Specialized for Iter<false>.

    bool hasNext() {
      while (data_offset_ + SIZE_OF_VALUE_SIZE_FIELD < data_size_) {
        const auto value_size = size();
        if (value_size == 0) return false;
        if (deleted()) {
          data_offset_ += SIZE_OF_VALUE_SIZE_FIELD + value_size;
        } else {
          return true;
        }
      }
      return false;
    }

    Bytes next() {
      const auto value = peekNext();
      data_offset_prev_ = data_offset_;
      data_offset_ += SIZE_OF_VALUE_SIZE_FIELD;
      data_offset_ += value.size();
      return value;
    }

    Bytes peekNext() const { return Bytes(data(), size()); }

    void markAsDeleted();
    // Specialized for Iter<false>.

    void toFront() {
      data_size_ = 0;
      data_offset_ = 0;
      data_offset_prev_ = 0;
    }

    Char* data() const { return tellGetAddress() + SIZE_OF_VALUE_SIZE_FIELD; }

    std::size_t size() const {
      std::int16_t size = 0;
      size += tellGetAddress()[0] << 8;
      size += tellGetAddress()[1];
      return size & 0x7fff;
    }

   private:
    Char* tellGetAddress() const { return data_ + data_offset_; }

    bool deleted() const { return *tellGetAddress() & 0x80; }

    Char* data_ = nullptr;
    std::uint32_t data_size_ = 0;
    std::uint32_t data_offset_ = 0;
    std::uint32_t data_offset_prev_ = 0;
    HasChanged has_changed_callback_;
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

  Iterator iterator();

  Iterator iterator(Iterator::HasChanged has_changed_callback);

  ConstIterator iterator() const;

  ConstIterator const_iterator() const;

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

static_assert(mt::hasExpectedSize<Block>(12, 16),
              "class Block does not have expected size");

template <>
inline Block::Iter<false>::Iter(char* data, std::size_t size,
                                HasChanged has_changed_callback)
    : data_(data),
      data_size_(size),
      has_changed_callback_(has_changed_callback) {
  MT_REQUIRE_NOT_NULL(data);
  MT_REQUIRE_TRUE(has_changed_callback);
}

template <>
inline void Block::Iter<false>::markAsDeleted() {
  char* prev_value_addr = data_ + data_offset_prev_;
  *prev_value_addr |= 0x80;
  if (has_changed_callback_) {
    has_changed_callback_();
  }
}

inline Block::Iterator Block::iterator() {
  // We do not use this->position_ to indicate the end of data, because
  // this member is always zero when the block is read from disk.
  return hasData() ? Iterator(data_, size_) : Iterator();
}

inline Block::Iterator Block::iterator(
    Iterator::HasChanged has_changed_callback) {
  // We do not use this->position_ to indicate the end of data, because
  // this member is always zero when the block is read from disk.
  return hasData() ? Iterator(data_, size_, has_changed_callback) : Iterator();
}

inline Block::ConstIterator Block::iterator() const { return const_iterator(); }

inline Block::ConstIterator Block::const_iterator() const {
  // We do not use this->position_ to indicate the end of data, because
  // this member is always zero when the block is read from disk.
  return hasData() ? ConstIterator(data_, size_) : ConstIterator();
}

bool operator==(const Block& lhs, const Block& rhs);

struct BlockWithId : Block {
  std::uint32_t id = -1;
  bool ignore = false;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INCLUDE_INTERNAL_BLOCK_HPP
