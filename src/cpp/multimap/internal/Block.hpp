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

#ifndef MULTIMAP_INTERNAL_BLOCK_HPP_INCLUDED
#define MULTIMAP_INTERNAL_BLOCK_HPP_INCLUDED

#include <algorithm>
#include <functional>
#include "multimap/internal/Arena.hpp"
#include "multimap/internal/Varint.hpp"
#include "multimap/thirdparty/mt.hpp"

namespace multimap {
namespace internal {

template <bool IsReadOnly> class BasicBlock {
  // This class wraps raw memory that is either writable or read-only.
  // The memory is neither owned nor deleted by objects of this class,
  // so ownership management must be implemented externally.
  // Objects of this class are copyable.

public:
  BasicBlock() = default;

  BasicBlock(char* data, std::size_t size) : data_(data), size_(size) {
    MT_REQUIRE_NOT_NULL(data_);
  }

  BasicBlock getView() const {
    return hasData() ? BasicBlock(data_, size_) : BasicBlock();
  }
  // Returns a shallow copy of the block whose offset is set to zero.

  bool hasData() const { return data_ != nullptr; }

  const char* data() const { return data_; }

  std::size_t size() const { return size_; }

  std::size_t offset() const { return offset_; }

  std::size_t remaining() const { return size_ - offset_; }

  void rewind() { offset_ = 0; }

  std::size_t readData(char* target, std::size_t size) {
    MT_REQUIRE_NOT_NULL(data_);
    const auto nbytes = std::min(size, remaining());
    std::memcpy(target, current(), nbytes);
    offset_ += nbytes;
    return nbytes;
  }
  // Copies up to `size` bytes into the buffer pointed to by `target`.
  // Returns the number of bytes actually copied which may be less than `size`.
  // Returns 0 if nothing could be extracted.

  std::size_t readSizeWithFlag(std::uint32_t* size, bool* flag) {
    MT_REQUIRE_NOT_NULL(data_);
    const auto nbytes =
        Varint::readUintWithFlag(current(), remaining(), size, flag);
    offset_ += nbytes;
    return nbytes;
  }
  // Reads a 32-bit unsigned integer and a boolean flag from the stream.
  // Returns the number of bytes copied.
  // Returns 0 if nothing could be extracted.

  // ---------------------------------------------------------------------------
  // The following interface is only enabled if IsReadOnly is true.

  MT_ENABLE_IF(IsReadOnly)
  BasicBlock(const char* data, std::size_t size) : data_(data), size_(size) {
    MT_REQUIRE_NOT_NULL(data_);
  }

  // ---------------------------------------------------------------------------
  // The following interface is only enabled if IsReadOnly is false.

  MT_ENABLE_IF(!IsReadOnly)
  char* data() { return data_; }

  MT_ENABLE_IF(!IsReadOnly)
  std::size_t writeData(const char* data, std::size_t size) {
    MT_REQUIRE_NOT_NULL(data_);
    const auto nbytes = std::min(size, remaining());
    std::memcpy(current(), data, nbytes);
    offset_ += nbytes;
    return nbytes;
  }

  MT_ENABLE_IF(!IsReadOnly)
  std::size_t writeSizeWithFlag(std::uint32_t size, bool flag) {
    MT_REQUIRE_NOT_NULL(data_);
    const auto nbytes =
        Varint::writeUintWithFlag(size, flag, current(), remaining());
    offset_ += nbytes;
    return nbytes;
  }

  bool writeFlagAt(bool flag, std::size_t offset) {
    MT_REQUIRE_NOT_NULL(data_);
    return offset < size_
               ? Varint::writeFlag(flag, data_ + offset, size_ - offset)
               : false;
  }

private:
  typedef typename std::conditional<IsReadOnly, const char, char>::type Char;

  Char* current() const { return data_ + offset_; }

  Char* data_ = nullptr;
  std::uint32_t size_ = 0;
  std::uint32_t offset_ = 0;
};

template <bool IsReadOnly>
struct ExtendedBasicBlock : public BasicBlock<IsReadOnly> {
  // This type extends `BasicBlock` with an `id` and `ignore` field.
  // As with the base class, objects of this type are copyable.

  typedef BasicBlock<IsReadOnly> Base;

  ExtendedBasicBlock() = default;

  ExtendedBasicBlock(const BasicBlock<IsReadOnly>& base) : Base(base) {}

  ExtendedBasicBlock(char* data, std::size_t size) : Base(data, size) {}

  ExtendedBasicBlock(char* data, std::size_t size, std::uint32_t id)
      : Base(data, size), id(id) {}

  MT_ENABLE_IF(IsReadOnly)
  ExtendedBasicBlock(const char* data, std::size_t size) : Base(data, size) {}

  MT_ENABLE_IF(IsReadOnly)
  ExtendedBasicBlock(const char* data, std::size_t size, std::uint32_t id)
      : Base(data, size), id(id) {}

  std::uint32_t id = -1;
  bool ignore = false;
};

// -----------------------------------------------------------------------------
// Typedefs

typedef BasicBlock<true> ReadOnlyBlock;
typedef BasicBlock<false> ReadWriteBlock;

typedef ExtendedBasicBlock<true> ExtendedReadOnlyBlock;
typedef ExtendedBasicBlock<false> ExtendedReadWriteBlock;

static_assert(mt::hasExpectedSize<ReadOnlyBlock>(12, 16),
              "class ReadOnlyBlock does not have expected size");

static_assert(mt::hasExpectedSize<ExtendedReadOnlyBlock>(20, 24),
              "class ReadOnlyBlock does not have expected size");

} // namespace internal
} // namespace multimap

#endif // MULTIMAP_INTERNAL_BLOCK_HPP_INCLUDED
