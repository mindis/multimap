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
public:
  BasicBlock() = default;

  BasicBlock(char* data, std::size_t size) : data_(data), size_(size) {
    MT_REQUIRE_NOT_NULL(data_);
  }

  BasicBlock(BasicBlock&&) = default;
  BasicBlock& operator=(BasicBlock&&) = default;

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

  typedef BasicBlock<IsReadOnly> Base;

  ExtendedBasicBlock() = default;

  ExtendedBasicBlock(ExtendedBasicBlock&&) = default;
  ExtendedBasicBlock& operator=(ExtendedBasicBlock&&) = default;

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

// class Block2 {
// public:
//  Block2() = default;

//  Block2(std::size_t size, Arena& arena);

//  const char* data() const { return data_; }
//  // Returns a read-only pointer to the wrapped data.

//  std::size_t size() const { return size_; }
//  // Returns the number of bytes wrapped.

//  bool empty() const { return size_ == 0; }
//  // Returns true if the number of bytes wrapped is zero, and false otherwise.

//  void resize(std::size_t size, Arena& arena) {
//    data_ = arena.allocate(size);
//    size_ = size;
//  }

//  void clear() { offset_ = 0; }

//  void copyFrom(const char* cstr) { copyFrom(cstr, std::strlen(cstr)); }

//  void copyFrom(const std::string& str) { copyFrom(str.data(), str.size()); }

//  void copyFrom(const void* data, std::size_t size) {
//    MT_REQUIRE_NOT_NULL(data);
//    MT_REQUIRE_EQ(size, size_);
//    std::memcpy(data_, data, size);
//  }

//  void copyTo(void* target) const { std::memcpy(target, data_, size_); }

//  std::size_t numBytesFree() const { return size_ - offset_; }

//  std::size_t numBytesUsed() const { return offset_; }

//  struct Meta {

//    Meta() = default;

//    Meta(std::uint32_t size) : size(size) {}

//    Meta(std::uint32_t size, bool removed) : size(size), removed(removed) {}

//    std::uint32_t size = 0;
//    bool removed = false;
//  };

//  // Preconditions:
//  //  * `meta.size` is less than or equal to `Varint32::MAX_VALUE_WITH_FLAG`.
//  bool writeMeta(const Meta& meta) {
//    const auto required = Varint32::numBytesRequired(meta.size, meta.removed);
//    MT_ASSERT_NE(required, -1);
//    if (static_cast<std::size_t>(required) <= numBytesFree()) {
//      const auto bytes_written =
//          Varint32::writeUint(meta.size, meta.removed, data_ + offset_);
//      MT_ASSERT_NE(bytes_written, -1);
//      offset_ += bytes_written;
//      return true;
//    }
//    return false;
//  }

//  bool writeData(const char* data, std::size_t size) {
//    if (size <= numBytesFree()) {
//      std::memcpy(data_ + offset_, data, size);
//      offset_ += size;
//      return true;
//    }
//    return false;
//  }

// private:
//  char* data_ = nullptr;
//  std::uint32_t size_ = 0;
//  std::uint32_t offset_ = 0;
//};

// struct ExtendedBlock : Block2 {

//  ExtendedBlock(const Block2& base) : Block2(base) {}

//  ExtendedBlock() : Block2(base) {}

//  std::uint32_t id = -1;
//  bool ignore = false;
//};

} // namespace internal
} // namespace multimap

#endif // MULTIMAP_INTERNAL_BLOCK_HPP_INCLUDED
