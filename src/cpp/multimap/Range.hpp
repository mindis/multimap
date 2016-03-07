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

// -----------------------------------------------------------------------------
// Documentation:  http://multimap.io/cppreference/#rangehpp
// -----------------------------------------------------------------------------

#ifndef MULTIMAP_RANGE_HPP_INCLUDED
#define MULTIMAP_RANGE_HPP_INCLUDED

#include <cstring>
#include <functional>
#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/thirdparty/xxhash/xxhash.h"
#include "multimap/Bytes.hpp"

namespace multimap {

class Range {
 public:
  Range() = default;

  Range(const char* cstr) : Range(cstr, std::strlen(cstr)) {}

  Range(const Bytes& bytes) : Range(bytes.data(), bytes.size()) {}

  Range(const void* data, size_t size)
      : Range(static_cast<const byte*>(data),
              static_cast<const byte*>(data) + size) {}

  Range(const byte* begin, const byte* end) : begin_(begin), end_(end) {}

  const byte* begin() const { return begin_; }

  const byte* end() const { return end_; }

  size_t size() const { return end_ - begin_; }

  void copyTo(Bytes* target) const {
    target->resize(size());
    std::memcpy(target->data(), begin_, target->size());
  }

  Bytes makeCopy() const {
    Bytes copy;
    copyTo(&copy);
    return copy;
  }

  template <typename Allocate>
  Range makeCopy(Allocate allocate) const {
    const size_t count = size();
    byte* data = allocate(count);
    std::memcpy(data, begin_, count);
    return Range(data, count);
  }

  // I/O Support
  // ---------------------------------------------------------------------------
  // Writing Range objects to a buffer or file stream is self-describing, i.e.
  // the size of the range in number of bytes is part of the serialization and
  // does not need to be maintained separately. This way ranges can be restored
  // by parsing the buffer or file stream, which can be done via class Bytes.
  // The encoding is as follows: [number of bytes as varint][actual data bytes]

  static Range readFromBuffer(const byte* buffer);

  static Range readFromStream(std::FILE* stream,
                              std::function<byte*(size_t)> allocate);

  byte* writeToBuffer(byte* begin, byte* end) const;
  // Writes the bytes to the buffer starting at `begin`. Returns a pointer into
  // the buffer past the last byte written which can be used for further write
  // operations. If there is no sufficient space in the buffer to write the
  // whole range, `begin` is returned.

  byte* writePartialToBuffer(byte* begin, byte* end) const;

  void writeToStream(std::FILE* stream) const;
  // Writes the bytes to a file stream or throws an exception if that was not
  // possible for some reason. In Multimap's I/O policy an unsuccessful write
  // to a file stream is considered a fatal error which only happens due to
  // faulty user behavior, e.g. someone deleted the file, or due to reaching
  // certain system limits, e.g. the device has no more space left.

  bool operator==(const Range& other) const {
    return (size() == other.size())
               ? std::memcmp(begin_, other.begin_, size()) == 0
               : false;
  }

  bool operator!=(const Range& other) const { return !(*this == other); }

  bool operator<(const Range& other) const {
    const size_t min_size = mt::min(size(), other.size());
    const int result = std::memcmp(begin_, other.begin_, min_size);
    return (result == 0) ? (size() < other.size()) : (result < 0);
  }

 private:
  static constexpr const byte* EMPTY = reinterpret_cast<const byte*>("");
  const byte* begin_ = EMPTY;
  const byte* end_ = EMPTY;
};

}  // namespace multimap

namespace std {

template <>
struct hash< ::multimap::Range> {
  size_t operator()(const ::multimap::Range& range) const {
    return mt::is64BitSystem() ? XXH64(range.begin(), range.size(), 0)
                               : XXH32(range.begin(), range.size(), 0);
  }
};

}  // namespace std

#endif  // MULTIMAP_RANGE_HPP_INCLUDED
