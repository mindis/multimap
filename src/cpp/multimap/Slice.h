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
// Documentation:  https://multimap.io/cppreference/#slicehpp
// -----------------------------------------------------------------------------

#ifndef MULTIMAP_SLICE_H_
#define MULTIMAP_SLICE_H_

#include <functional>
#include <string>
#include "multimap/thirdparty/mt/common.h"
#include "multimap/thirdparty/xxhash/xxhash.h"
#include "multimap/Arena.h"
#include "multimap/Bytes.h"

namespace multimap {

class Slice {
 public:
  Slice() = default;

  Slice(const char* cstr) : Slice(cstr, std::strlen(cstr)) {}  // NOLINT

  Slice(const Bytes& bytes) : Slice(bytes.data(), bytes.size()) {}  // NOLINT

  Slice(const std::string& str) : Slice(str.data(), str.size()) {}  // NOLINT

  Slice(const void* data, size_t size)
      : data_(static_cast<const byte*>(data)), size_(size) {}

  const byte* data() const { return data_; }

  size_t size() const { return size_; }

  bool empty() const { return size_ == 0; }

  void clear() { *this = Slice(); }

  const byte* begin() const { return data_; }

  const byte* end() const { return data_ + size_; }

  void copyTo(Bytes* target) const {
    target->resize(size_);
    std::memcpy(target->data(), data_, size_);
  }

  Bytes makeCopy() const {
    Bytes copy;
    copyTo(&copy);
    return copy;
  }

  Slice makeCopy(Arena* arena) const {
    byte* new_data = arena->allocate(size_);
    std::memcpy(new_data, data_, size_);
    return Slice(new_data, size_);
  }

  static Slice makeCopy(const Bytes& bytes, Arena* arena) {
    return Slice(bytes).makeCopy(arena);
  }

  std::string toString() const {
    return std::string(reinterpret_cast<const char*>(data_), size_);
  }

  // I/O Support
  // ---------------------------------------------------------------------------
  // Writing Slice objects to a buffer or file stream is self-describing, i.e.
  // the size of the slice in number of bytes is part of the serialization and
  // does not need to be maintained separately. This way slices can be restored
  // parsing the buffer or file stream. The latter can be done via class Bytes.
  // The encoding is as follows: [number of bytes as varint][actual data bytes]

  static Slice readFromBuffer(const byte* buffer);
  // Reads a slice from a buffer. The returned slice is backed by the given
  // buffer. The caller has to guarantee that `buffer` points to a valid
  // encoding of a slice once written via `writeToBuffer()`. The end()-pointer
  // of the returned slice must be used as the input for subsequent reads.

  size_t writeToBuffer(byte* begin, byte* end) const;
  // Writes the slice to the buffer starting at `begin`. Returns the number
  // of bytes written, which may be zero if there was not sufficient space.

  void writeToStream(std::ostream* stream) const;
  // Writes the slice to a file stream or throws an exception if that was not
  // possible for some reason. In Multimap's I/O policy an unsuccessful write
  // to a file stream is considered a fatal error which only happens due to
  // faulty user behavior, e.g. someone deleted the file, or due to reaching
  // certain system limits, e.g. the device has no more space left.

 private:
  const byte* data_ = reinterpret_cast<const byte*>("");
  size_t size_ = 0;
};

inline bool operator==(const Slice& a, const Slice& b) {
  return internal::equal(a.data(), a.size(), b.data(), b.size());
}

inline bool operator!=(const Slice& a, const Slice& b) { return !(a == b); }

inline bool operator<(const Slice& a, const Slice& b) {
  return internal::less(a.data(), a.size(), b.data(), b.size());
}

inline bool operator>=(const Slice& a, const Slice& b) { return !(a < b); }

}  // namespace multimap

namespace std {

template <>
struct hash< ::multimap::Slice> {
  size_t operator()(const ::multimap::Slice& range) const {
    return mt::is64BitSystem() ? XXH64(range.data(), range.size(), 0)
                               : XXH32(range.data(), range.size(), 0);
  }
};

}  // namespace std

#endif  // MULTIMAP_SLICE_H_
