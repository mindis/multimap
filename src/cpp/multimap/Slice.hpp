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
// Documentation:  http://multimap.io/cppreference/#slicehpp
// -----------------------------------------------------------------------------

#ifndef MULTIMAP_SLICE_HPP_INCLUDED
#define MULTIMAP_SLICE_HPP_INCLUDED

#include <functional>
#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/thirdparty/xxhash/xxhash.h"
#include "multimap/Bytes.hpp"

namespace multimap {

class Slice {
 public:
  Slice() = default;

  Slice(const char* cstr) : Slice(cstr, std::strlen(cstr)) {}

  Slice(const Bytes& bytes) : Slice(bytes.data(), bytes.size()) {}

  Slice(const std::string& str) : Slice(str.data(), str.size()) {}

  Slice(const void* data, size_t size)
      : Slice(static_cast<const byte*>(data),
              static_cast<const byte*>(data) + size) {}

  Slice(const byte* begin, const byte* end) : beg_(begin), end_(end) {}

  const byte* begin() const { return beg_; }

  const byte* end() const { return end_; }

  size_t size() const { return end_ - beg_; }

  bool empty() const { return beg_ == end_; }

  void copyTo(Bytes* target) const {
    target->resize(size());
    std::memcpy(target->data(), beg_, target->size());
  }

  Bytes makeCopy() const {
    Bytes copy;
    copyTo(&copy);
    return copy;
  }

  template <typename Allocate>
  Slice makeCopy(Allocate allocate) const {
    const size_t count = size();
    byte* data = allocate(count);
    std::memcpy(data, beg_, count);
    return Slice(data, count);
  }

  std::string toString() const {
    return std::string(reinterpret_cast<const char*>(beg_), size());
  }

  // I/O Support
  // ---------------------------------------------------------------------------
  // Writing Slice objects to a buffer or file stream is self-describing, i.e.
  // the size of the range in number of bytes is part of the serialization and
  // does not need to be maintained separately. This way ranges can be restored
  // parsing the buffer or file stream. The latter can be done via class Bytes.
  // The encoding is as follows: [number of bytes as varint][actual data bytes]

  static Slice readFromBuffer(const byte* buffer);

  static Slice readFromStream(std::FILE* stream,
                              std::function<byte*(size_t)> allocate);

  byte* writeToBuffer(byte* begin, byte* end) const;
  // Writes the bytes to the buffer starting at `begin`. Returns a pointer into
  // the buffer past the last byte written which can be used for further write
  // operations. If there is no sufficient space in the buffer to write the
  // whole range, `begin` is returned.

  void writeToStream(std::FILE* stream) const;
  // Writes the bytes to a file stream or throws an exception if that was not
  // possible for some reason. In Multimap's I/O policy an unsuccessful write
  // to a file stream is considered a fatal error which only happens due to
  // faulty user behavior, e.g. someone deleted the file, or due to reaching
  // certain system limits, e.g. the device has no more space left.

 private:
  static const byte* EMPTY;
  const byte* beg_ = EMPTY;
  const byte* end_ = EMPTY;
};

inline bool operator==(const Slice& a, const Slice& b) {
  return internal::equal(a.begin(), a.size(), b.begin(), b.size());
}

inline bool operator!=(const Slice& a, const Slice& b) { return !(a == b); }

inline bool operator<(const Slice& a, const Slice& b) {
  return internal::less(a.begin(), a.size(), b.begin(), b.size());
}

}  // namespace multimap

namespace std {

template <>
struct hash< ::multimap::Slice> {
  size_t operator()(const ::multimap::Slice& range) const {
    return mt::is64BitSystem() ? XXH64(range.begin(), range.size(), 0)
                               : XXH32(range.begin(), range.size(), 0);
  }
};

}  // namespace std

#endif  // MULTIMAP_SLICE_HPP_INCLUDED
