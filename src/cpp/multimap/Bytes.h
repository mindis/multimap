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
// Documentation:  https://multimap.io/cppreference/#byteshpp
// -----------------------------------------------------------------------------

#ifndef MULTIMAP_BYTES_H_
#define MULTIMAP_BYTES_H_

#include <cstdio>
#include <string>
#include "multimap/thirdparty/mt/common.h"

namespace multimap {

typedef mt::byte byte;

typedef mt::Bytes Bytes;

Bytes toBytes(const char* cstr);

Bytes toBytes(const std::string& str);

size_t readBytesFromBuffer(const byte* buffer, Bytes* output);

size_t writeBytesToBuffer(const Bytes& input, byte* begin, byte* end);

bool readBytesFromStream(std::FILE* stream, Bytes* output);

void writeBytesToStream(const Bytes& input, std::FILE* stream);

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

struct BytesEqual {
  bool operator()(const Bytes& a, const Bytes& b) const {
    return internal::equal(a.data(), a.size(), b.data(), b.size());
  }
};

struct BytesLess {
  // Comparator to be used with std::sort or similar functions.
  // We don't specialize operator "<" for Bytes, because doing so would require
  // to add this specialization to namespace std where class vector is defined.
  // Changing implicit behavior of standard library types is a very bad thing.
  //
  // Not using this comparator in std::sort or related functions, will trigger
  // std::lexicographical_compare, which works similar, but may be slower.

  bool operator()(const Bytes& a, const Bytes& b) const {
    return internal::less(a.data(), a.size(), b.data(), b.size());
  }
};

}  // namespace multimap

#endif  // MULTIMAP_BYTES_H_
