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
// Documentation:  http://multimap.io/cppreference/#byteshpp
// -----------------------------------------------------------------------------

#ifndef MULTIMAP_BYTES_HPP_INCLUDED
#define MULTIMAP_BYTES_HPP_INCLUDED

#include <algorithm>
#include <cstring>
#include <functional>
#include <string>
#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/thirdparty/xxhash/xxhash.h"

namespace multimap {

class Bytes {
 public:
  Bytes() : data_(""), size_(0) {}

  Bytes(const char* cstr) : Bytes(cstr, std::strlen(cstr)) {}

  Bytes(const std::string& str) : Bytes(str.data(), str.size()) {}

  Bytes(const void* data, size_t size)
      : data_(static_cast<const char*>(data)), size_(size) {}

  const char* data() const { return data_; }

  size_t size() const { return size_; }

  const char* begin() const { return data_; }

  const char* end() const { return data_ + size_; }

  bool empty() const { return size_ == 0; }

  void clear() {
    data_ = "";
    size_ = 0;
  }

  std::string toString() const { return std::string(data_, size_); }

 private:
  const char* data_;
  size_t size_;
};

inline bool operator==(const Bytes& lhs, const Bytes& rhs) {
  return (lhs.size() == rhs.size())
             ? std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0
             : false;
}

inline bool operator!=(const Bytes& lhs, const Bytes& rhs) {
  return !(lhs == rhs);
}

inline bool operator<(const Bytes& lhs, const Bytes& rhs) {
  const auto min_size = std::min(lhs.size(), rhs.size());
  const auto result = std::memcmp(lhs.data(), rhs.data(), min_size);
  return (result == 0) ? (lhs.size() < rhs.size()) : (result < 0);
}

}  // namespace multimap

namespace std {

template <>
struct hash< ::multimap::Bytes> {
  size_t operator()(const ::multimap::Bytes& bytes) const {
    return mt::is64BitSystem() ? XXH64(bytes.data(), bytes.size(), 0)
                               : XXH32(bytes.data(), bytes.size(), 0);
    // Compiler will optimize out branching.
  }
};

}  // namespace std

#endif  // MULTIMAP_BYTES_HPP_INCLUDED
