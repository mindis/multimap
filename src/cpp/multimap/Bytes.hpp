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

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace multimap {

typedef unsigned char byte;

typedef std::vector<byte> Bytes;

Bytes toBytes(const char* cstr);

Bytes toBytes(const std::string& str);

size_t readBytesFromBuffer(const byte* buffer, Bytes* output);

size_t readBytesFromStream(std::FILE* stream, Bytes* output);

size_t writeBytesToBuffer(byte* begin, byte* end, const Bytes& input);

size_t writeBytesToStream(std::FILE* stream, const Bytes& input);

namespace internal {

inline bool equal(const byte* a, size_t alen, const byte* b, size_t blen) {
  return (alen == blen) ? (std::memcmp(a, b, alen) == 0) : false;
}

inline bool less(const byte* a, size_t alen, const byte* b, size_t blen) {
  const size_t minlen = alen < blen ? alen : blen;
  const int result = std::memcmp(a, b, minlen);
  return (result == 0) ? (alen < blen) : (result < 0);
}

}  // namespace internal

struct Equal {
  bool operator()(const Bytes& a, const Bytes& b) const {
    return internal::equal(a.data(), a.size(), b.data(), b.size());
  }
};

struct Less {
  // Comparator to be used with std::sort or similar functions.
  // We don't specialize operator "<" for Bytes, because doing so would require
  // to add this specialization to namespace std where class vector is defined.
  // Overriding behavior in namespace std is a very bad thing.
  //
  // Not using this comparator in std::sort or related functions, will trigger
  // std::lexicographical_compare, which works similar, but may be slower.

  bool operator()(const Bytes& a, const Bytes& b) const {
    return internal::less(a.data(), a.size(), b.data(), b.size());
  }
};

}  // namespace multimap

#endif  // MULTIMAP_BYTES_HPP_INCLUDED
