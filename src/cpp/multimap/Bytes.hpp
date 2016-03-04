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
#include <functional>
#include <string>
#include "multimap/internal/Varint.hpp"
#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/thirdparty/xxhash/xxhash.h"

namespace multimap {

class Bytes {
 public:
  Bytes() = default;

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
  const char* data_ = "";
  size_t size_ = 0;
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

template <typename Allocate>
Bytes readBytes(std::FILE* stream, Allocate allocate) {
  uint32_t size;
  if (internal::Varint::readUintFromStream(stream, &size) != 0) {
    const auto data = allocate(size);
    mt::fread(stream, data, size);
    // The stream is expected to contain valid encodings of Bytes objects.
    // Hence, after successfully reading the size field, mt::fread() will
    // throw, if the data field could not be read to signal an invalid stream.
    return Bytes(data, size);
  }
  return Bytes();
}

inline bool readBytes(std::FILE* stream, std::vector<char>* bytes) {
  uint32_t size;
  if (internal::Varint::readUintFromStream(stream, &size) != 0) {
    bytes->resize(size);
    mt::fread(stream, bytes->data(), bytes->size());
    // Read comment above, why mt::fread() may throw here.
    return true;
  }
  return false;
}

inline void writeBytes(std::FILE* stream, const Bytes& bytes) {
  mt::Check::notZero(internal::Varint::writeUintToStream(stream, bytes.size()),
                     "writeUintToStream() failed");
  mt::fwrite(stream, bytes.data(), bytes.size());
  // This method throws if any of the write operations fail.
  // In Multimap's philosophy there is no reasonable recovery if a bytes object
  // could not be written. If that happens, it is either because of faulty user
  // behavious, e.g. someone deleted the file, or because there is simply not
  // enough space left on the device.
}

inline void writeBytes(std::FILE* stream, const std::vector<char>& bytes) {
  writeBytes(stream, Bytes(bytes.data(), bytes.size()));
}

}  // namespace multimap

namespace std {

template <>
struct hash< ::multimap::Bytes> {
  size_t operator()(const ::multimap::Bytes& bytes) const {
    return mt::is64BitSystem() ? XXH64(bytes.data(), bytes.size(), 0)
                               : XXH32(bytes.data(), bytes.size(), 0);
  }
};

}  // namespace std

#endif  // MULTIMAP_BYTES_HPP_INCLUDED
