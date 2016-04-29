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

const uint32_t Varint::MAX_VARINT_WITH_FLAG = (1 << 29) - 1;

size_t Varint::readFromBuffer(const byte* begin, const byte* end,
                              uint32_t* value, bool* flag) {
  const auto remaining = end - begin;
  if (remaining > 0) {
    *flag = *begin & 0x20;              // 00100000
    const auto length = *begin & 0xC0;  // 11000000
    switch (length) {
      case 0x00:  // 00000000
        *value = *begin++ & 0x1F;
        return 1;
      case 0x40:  // 01000000
        if (remaining < 2) return 0;
        *value = (*begin++ & 0x1F) << 8;
        *value += *begin++;
        return 2;
      case 0x80:  // 10000000
        if (remaining < 3) return 0;
        *value = (*begin++ & 0x1F) << 16;
        *value += *begin++ << 8;
        *value += *begin++;
        return 3;
      case 0xC0:  // 11000000
        if (remaining < 4) return 0;
        *value = (*begin++ & 0x1F) << 24;
        *value += *begin++ << 16;
        *value += *begin++ << 8;
        *value += *begin++;
        return 4;
      default:
        MT_FAIL("Reached default branch in switch statement");
    }
  }
  return 0;
}

size_t Varint::writeToBuffer(byte* begin, byte* end, uint32_t value,
                             bool flag) {
  const auto remaining = end - begin;
  if (value < 0x00000020) {  // 00000000 00000000 00000000 00100000
    if (remaining < 1) return 0;
    *begin++ = value | (flag ? 0x20 : 0x00);
    return 1;
  }
  if (value < 0x00002000) {  // 00000000 00000000 00100000 00000000
    if (remaining < 2) return 0;
    *begin++ = (value >> 8) | (flag ? 0x60 : 0x40);
    *begin++ = (value);
    return 2;
  }
  if (value < 0x00200000) {  // 00000000 00100000 00000000 00000000
    if (remaining < 3) return 0;
    *begin++ = (value >> 16) | (flag ? 0xA0 : 0x80);
    *begin++ = (value >> 8);
    *begin++ = (value);
    return 3;
  }
  if (value < 0x20000000) {  // 00100000 00000000 00000000 00000000
    if (remaining < 4) return 0;
    *begin++ = (value >> 24) | (flag ? 0xE0 : 0xC0);
    *begin++ = (value >> 16);
    *begin++ = (value >> 8);
    *begin++ = (value);
    return 4;
  }
  mt::fail("Varint::writeToBuffer() failed. Too big value: %d", value);
  return 0;
}

void Varint::setFlagInBuffer(byte* buffer, bool flag) {
  flag ? (*buffer |= 0x20) : (*buffer &= 0xDF);
}

}  // namespace internal
}  // namespace multimap
