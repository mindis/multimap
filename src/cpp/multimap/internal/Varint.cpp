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

#include "multimap/internal/Varint.h"

#include "multimap/thirdparty/mt/assert.h"
#include "multimap/thirdparty/mt/check.h"
#include "multimap/thirdparty/mt/fileio.h"

namespace multimap {
namespace internal {

const int MAX_VARINT32_BYTES = 5;

size_t Varint::readFromBuffer(const byte* begin, const byte* end,
                              uint32_t* value, bool* flag) {
  *value = 0;
  int shift = 6;
  if (MT_LIKELY((end - begin) >= MAX_VARINT32_BYTES)) {
    *value = *begin & 0x3F;
    *flag = *begin & 0x40;
    if (!(*begin++ & 0x80)) return 1;
    for (int i = 1; i < MAX_VARINT32_BYTES; i++) {
      *value += static_cast<uint32_t>(*begin & 0x7F) << shift;
      if (!(*begin++ & 0x80)) return i + 1;
      shift += 7;
    }
  } else if (begin < end) {
    *value = *begin & 0x3F;
    *flag = *begin & 0x40;
    if (!(*begin++ & 0x80)) return 1;
    for (int i = 1; (begin < end) && (i < MAX_VARINT32_BYTES); i++) {
      *value += static_cast<uint32_t>(*begin & 0x7F) << shift;
      if (!(*begin++ & 0x80)) return i + 1;
      shift += 7;
    }
  }
  return 0;
}

size_t Varint::writeToBuffer(byte* begin, byte* end, uint32_t value,
                             bool flag) {
  byte* pos = begin;
  if (pos != end) {
    *pos = (flag ? 0x40 : 0) | (value & 0x3F);
    if (value > 0x3F) {
      *pos++ |= 0x80;
      value >>= 6;
      while ((pos != end) && (value > 0x7F)) {
        *pos++ = 0x80 | (value & 0x7F);
        value >>= 7;
      }
      if (pos == end) return 0;  // Insufficient space in buffer.
      *pos++ = value;
    } else {
      pos++;
    }
  }
  return pos - begin;
}

void Varint::setFlagInBuffer(byte* buffer, bool flag) {
  flag ? (*buffer |= 0x40) : (*buffer &= 0xBF);
}

}  // namespace internal
}  // namespace multimap
