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

#ifndef MULTIMAP_INTERNAL_BLOCK_HPP_INCLUDED
#define MULTIMAP_INTERNAL_BLOCK_HPP_INCLUDED

#include <cstring>
#include <algorithm>
#include "multimap/internal/Varint.hpp"
#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/Range.hpp"

namespace multimap {
namespace internal {

template <bool IsReadOnly>
class Block {
  // This class wraps raw memory that is either writable or read-only.
  // The memory is neither owned nor deleted by objects of this class,
  // so ownership management must be implemented externally.
  // Objects of this class are copyable.

 public:
  Block() = default;

  Block(byte* data, size_t size) : data_(data), size_(size) {
    MT_REQUIRE_NOT_NULL(data_);
  }

  Block getView() const { return hasData() ? Block(data_, size_) : Block(); }
  // Returns a shallow copy of the block whose offset is set to zero.

  bool hasData() const { return data_ != nullptr; }

  const byte* data() const { return data_; }

  size_t size() const { return size_; }

  size_t offset() const { return offset_; }

  size_t remaining() const { return size_ - offset_; }

  void rewind() { offset_ = 0; }

  size_t readData(size_t size, byte* target) {
    MT_REQUIRE_NOT_NULL(data_);
    const auto nbytes = std::min(size, remaining());
    std::memcpy(target, pos(), nbytes);
    offset_ += nbytes;
    return nbytes;
  }
  // Copies up to `size` bytes into the buffer pointed to by `target`.
  // Returns the number of bytes actually copied which may be less than `size`.
  // Returns 0 if nothing could be extracted.

  bool readSizeWithFlag(uint32_t* size, bool* flag) {
    MT_REQUIRE_NOT_NULL(data_);
    if (pos() != end()) {
      const byte* new_pos = Varint::readFromBuffer(pos(), size, flag);
      offset_ = new_pos - data_;
      return new_pos != end();
    }
    return false;
  }
  // Reads a 32-bit unsigned integer and a boolean flag from the stream.
  // Returns the number of bytes copied.
  // Returns 0 if nothing could be extracted.

  // ---------------------------------------------------------------------------
  // The following interface is only enabled if IsReadOnly is true.
  // ---------------------------------------------------------------------------

  MT_ENABLE_IF(IsReadOnly)
  explicit Block(const char* cstr) : Block(cstr, std::strlen(cstr)) {}

  MT_ENABLE_IF(IsReadOnly)
  Block(const byte* data, size_t size) : data_(data), size_(size) {
    MT_REQUIRE_NOT_NULL(data_);
  }

  // ---------------------------------------------------------------------------
  // The following interface is only enabled if IsReadOnly is false.
  // ---------------------------------------------------------------------------

  MT_DISABLE_IF(IsReadOnly)
  byte* data() { return data_; }

  MT_DISABLE_IF(IsReadOnly)
  void fillUpWithZeros() { std::memset(pos(), 0, remaining()); }

  MT_DISABLE_IF(IsReadOnly)
  size_t writeData(const Range& bytes) {
    const auto nbytes = std::min(bytes.size(), remaining());
    std::memcpy(pos(), bytes.begin(), nbytes);
    offset_ += nbytes;
    return nbytes;
  }

  MT_DISABLE_IF(IsReadOnly)
  bool writeSizeWithFlag(uint32_t size, bool flag) {
    const byte* new_pos = Varint::writeToBuffer(pos(), end(), size, flag);
    offset_ = new_pos - data_;
    return new_pos != end();
  }

  MT_DISABLE_IF(IsReadOnly)
  void setFlagAt(bool flag, size_t offset) {
    Varint::setFlagInBuffer(data_ + offset, flag);
  }

 private:
  typedef typename std::conditional<IsReadOnly, const byte, byte>::type Byte;

  Byte* pos() const { return data_ + offset_; }
  Byte* end() const { return data_ + size_; }

  Byte* data_ = nullptr;
  uint32_t offset_ = 0;
  uint32_t size_ = 0;
};

template <bool IsReadOnly>
struct ExtendedBlock : public Block<IsReadOnly> {
  // This type extends `Block` with an `id` and `ignore` field.
  // As with the base class, objects of this type are copyable.

  typedef Block<IsReadOnly> Base;

  ExtendedBlock() = default;

  ExtendedBlock(const Block<IsReadOnly>& base) : Base(base) {}

  ExtendedBlock(byte* data, size_t size) : Base(data, size) {}

  ExtendedBlock(byte* data, size_t size, uint32_t id)
      : Base(data, size), id(id) {}

  MT_ENABLE_IF(IsReadOnly)
  ExtendedBlock(const byte* data, size_t size) : Base(data, size) {}

  MT_ENABLE_IF(IsReadOnly)
  ExtendedBlock(const byte* data, size_t size, uint32_t id)
      : Base(data, size), id(id) {}

  uint32_t id = -1;
  bool ignore = false;
};

// -----------------------------------------------------------------------------
// Typedefs
// -----------------------------------------------------------------------------

typedef Block<true> ReadOnlyBlock;
typedef Block<false> ReadWriteBlock;

typedef ExtendedBlock<true> ExtendedReadOnlyBlock;
typedef ExtendedBlock<false> ExtendedReadWriteBlock;

static_assert(mt::hasExpectedSize<ReadOnlyBlock>(12, 16),
              "class ReadOnlyBlock does not have expected size");

static_assert(mt::hasExpectedSize<ExtendedReadOnlyBlock>(20, 24),
              "class ExtendedReadOnlyBlock does not have expected size");

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_BLOCK_HPP_INCLUDED
