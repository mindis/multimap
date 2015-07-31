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

#ifndef MULTIMAP_BYTES_HPP
#define MULTIMAP_BYTES_HPP

#include <algorithm>
#include <cstring>
#include <string>

namespace multimap {

typedef unsigned char byte;

// A type that wraps a byte array and its size without taking ownership.
class Bytes {
 public:
  Bytes() : data_(nullptr), size_(0) {}

  Bytes(const char* cstr) : Bytes(cstr, std::strlen(cstr)) {}

  Bytes(const std::string& str) : Bytes(str.data(), str.size()) {}

  Bytes(const void* data, std::size_t size) : data_(data), size_(size) {}

  const void* data() const { return data_; }

  std::size_t size() const { return size_; }

  bool empty() const { return size_ == 0; }

  void clear() {
    data_ = nullptr;
    size_ = 0;
  }

  std::string ToString() const {
    return std::string(static_cast<const char*>(data_), size_);
  }

 private:
  const void* data_;
  std::size_t size_;
};

inline bool Equal(const Bytes& lhs, const Bytes& rhs) {
  return (lhs.size() == rhs.size())
             ? std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0
             : false;
}

inline bool operator==(const Bytes& lhs, const Bytes& rhs) {
  return Equal(lhs, rhs);
}

inline bool operator!=(const Bytes& lhs, const Bytes& rhs) {
  return !(lhs == rhs);
}

inline bool Less(const Bytes& lhs, const Bytes& rhs) {
  const auto min_size = std::min(lhs.size(), rhs.size());
  const auto result = std::memcmp(lhs.data(), rhs.data(), min_size);
  return (result == 0) ? (lhs.size() < rhs.size()) : (result < 0);
}

inline bool operator<(const Bytes& lhs, const Bytes& rhs) {
  return Less(lhs, rhs);
}

}  // namespace multimap

#endif  // MULTIMAP_BYTES_HPP
