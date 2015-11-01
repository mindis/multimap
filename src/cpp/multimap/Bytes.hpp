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

#ifndef MULTIMAP_BYTES_HPP_INCLUDED
#define MULTIMAP_BYTES_HPP_INCLUDED

#include <algorithm>
#include <cstring>
#include <functional>
#include <string>
#include "multimap/thirdparty/mt.hpp"
#include "multimap/thirdparty/xxhash.h"

namespace multimap {

// This class is a wrapper for raw byte data without ownership management.
class Bytes {
public:
  Bytes() : data_(""), size_(0) {}
  // Creates an instance that refers to an empty array.
  // Postconditions:
  //   * data() != nullptr
  //   * size() == 0

  Bytes(const char* cstr) : Bytes(cstr, std::strlen(cstr)) {}
  // Creates an instance that wraps a null-terminated C-string.
  // Preconditions:
  //   * cstr != nullptr
  // Postconditions:
  //   * data() == cstr
  //   * size() == std::strlen(cstr)

  Bytes(const std::string& str) : Bytes(str.data(), str.size()) {}
  // Creates an instance that wraps a standard string.
  // Postconditions:
  //   * data() == str.data()
  //   * size() == str.size()

  Bytes(const void* data, std::size_t size)
      : data_(static_cast<const char*>(data)), size_(size) {}
  // Creates an instance that wraps a pointer to data of size bytes.
  // Preconditions:
  //   * data != nullptr
  // Postconditions:
  //   * data() == data
  //   * size() == size

  const char* data() const { return data_; }
  // Returns a read-only pointer to the wrapped data.

  std::size_t size() const { return size_; }
  // Returns the number of bytes wrapped.

  bool empty() const { return size_ == 0; }
  // Returns true if the number of bytes wrapped is zero, and false otherwise.

  void clear() {
    data_ = "";
    size_ = 0;
  }
  // Let this instance refer to an empty array.
  // Postconditions:
  //   * data() != nullptr
  //   * size() == 0

  std::string toString() const { return std::string(data_, size_); }
  // Returns a deep copy of the wrapped data. std::string is used here as a
  // convenient byte buffer which may contain characters that are not printable.
  // Postconditions:
  //   * data() != result.data()
  //   * size() == result.size()

private:
  const char* data_;
  std::size_t size_;
};

inline bool operator==(const Bytes& lhs, const Bytes& rhs) {
  return (lhs.size() == rhs.size())
             ? std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0
             : false;
}
// Compares two `Bytes` objects for equality (byte-wise comparison), not for
// identity (pointer comparison). Returns `true` if the bytes in `lhs` and `rhs`
// are equal, `false` otherwise.

inline bool operator!=(const Bytes& lhs, const Bytes& rhs) {
  return !(lhs == rhs);
}
// Returns true if the bytes wrapped by lhs and rhs are not equal after
// byte-wise comparison. Returns false otherwise.

inline bool operator<(const Bytes& lhs, const Bytes& rhs) {
  const auto min_size = std::min(lhs.size(), rhs.size());
  const auto result = std::memcmp(lhs.data(), rhs.data(), min_size);
  return (result == 0) ? (lhs.size() < rhs.size()) : (result < 0);
}
// Returns true if lhs is less than rhs according to std::memcmp, and false
// otherwise. If lhs and rhs do not wrap the same number of bytes, only the
// first std::min(lhs.size(), rhs.size()) bytes will be compared.

} // namespace multimap

namespace std {

template <> struct hash< ::multimap::Bytes> {
  size_t operator()(const ::multimap::Bytes& bytes) const {
    return mt::is64BitSystem() ? XXH64(bytes.data(), bytes.size(), 0)
                               : XXH32(bytes.data(), bytes.size(), 0);
    // The compiler will optimize out branching here.
  }
};

} // namespace std

#endif // MULTIMAP_BYTES_HPP_INCLUDED
