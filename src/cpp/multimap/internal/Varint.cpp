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

#include "multimap/internal/Varint.hpp"

#include "multimap/thirdparty/mt/mt.hpp"

namespace multimap {
namespace internal {

const uint32_t Varint::Limits::MIN_N1 = 0;
const uint32_t Varint::Limits::MIN_N2 = (1 << 6);
const uint32_t Varint::Limits::MIN_N3 = (1 << 14);
const uint32_t Varint::Limits::MIN_N4 = (1 << 22);

const uint32_t Varint::Limits::MAX_N1 = (1 << 6) - 1;
const uint32_t Varint::Limits::MAX_N2 = (1 << 14) - 1;
const uint32_t Varint::Limits::MAX_N3 = (1 << 22) - 1;
const uint32_t Varint::Limits::MAX_N4 = (1 << 30) - 1;

const uint32_t Varint::Limits::MIN_N1_WITH_FLAG = 0;
const uint32_t Varint::Limits::MIN_N2_WITH_FLAG = (1 << 5);
const uint32_t Varint::Limits::MIN_N3_WITH_FLAG = (1 << 13);
const uint32_t Varint::Limits::MIN_N4_WITH_FLAG = (1 << 21);

const uint32_t Varint::Limits::MAX_N1_WITH_FLAG = (1 << 5) - 1;
const uint32_t Varint::Limits::MAX_N2_WITH_FLAG = (1 << 13) - 1;
const uint32_t Varint::Limits::MAX_N3_WITH_FLAG = (1 << 21) - 1;
const uint32_t Varint::Limits::MAX_N4_WITH_FLAG = (1 << 29) - 1;

size_t Varint::readFromBuffer(const byte* begin, const byte* end,
                              uint32_t* value) {
  const auto remaining = end - begin;
  if (remaining > 0) {
    const auto length = *begin & 0xC0;  // 11000000
    switch (length) {
      case 0x00:  // 00000000
        *value = *begin++;
        return 1;
      case 0x40:  // 01000000
        if (remaining < 2) return 0;
        *value = (*begin++ & 0x3F) << 8;
        *value += *begin++;
        return 2;
      case 0x80:  // 10000000
        if (remaining < 3) return 0;
        *value = (*begin++ & 0x3F) << 16;
        *value += *begin++ << 8;
        *value += *begin++;
        return 3;
      case 0xC0:  // 11000000
        if (remaining < 4) return 0;
        *value = (*begin++ & 0x3F) << 24;
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

// TODO Micro-benchmark this compared to byte[4].
size_t Varint::readFromStream(std::FILE* stream, uint32_t* value) {
  byte b0 = 0;
  if (mt::tryGet(stream, &b0)) {
    const auto length = b0 & 0xC0;  // 11000000
    switch (length) {
      case 0x00: {  // 00000000
        *value = b0;
        return 1;
      }
      case 0x40: {  // 01000000
        const byte b1 = mt::get(stream);
        *value = (b0 & 0x3F) << 8;
        *value += b1;
        return 2;
      }
      case 0x80: {  // 10000000
        const byte b1 = mt::get(stream);
        const byte b2 = mt::get(stream);
        *value = (b0 & 0x3F) << 16;
        *value += b1 << 8;
        *value += b2;
        return 3;
      }
      case 0xC0: {  // 11000000
        const byte b1 = mt::get(stream);
        const byte b2 = mt::get(stream);
        const byte b3 = mt::get(stream);
        *value = (b0 & 0x3F) << 24;
        *value += b1 << 16;
        *value += b2 << 8;
        *value += b3;
        return 4;
      }
      default:
        MT_FAIL("Reached default branch in switch statement");
    }
  }
  return 0;
}

size_t Varint::writeToBuffer(byte* begin, byte* end, uint32_t value) {
  const auto remaining = end - begin;
  if (value < 0x00000040) {  // 00000000 00000000 00000000 01000000
    if (remaining < 1) return 0;
    *begin++ = value;
    return 1;
  }
  if (value < 0x00004000) {  // 00000000 00000000 01000000 00000000
    if (remaining < 2) return 0;
    *begin++ = (value >> 8) | 0x40;
    *begin++ = (value);
    return 2;
  }
  if (value < 0x00400000) {  // 00000000 01000000 00000000 00000000
    if (remaining < 3) return 0;
    *begin++ = (value >> 16) | 0x80;
    *begin++ = (value >> 8);
    *begin++ = (value);
    return 3;
  }
  if (value < 0x40000000) {  // 01000000 00000000 00000000 00000000
    if (remaining < 4) return 0;
    *begin++ = (value >> 24) | 0xC0;
    *begin++ = (value >> 16);
    *begin++ = (value >> 8);
    *begin++ = (value);
    return 4;
  }
  mt::fail("Varint::writeToBuffer() failed. Too big value: %d", value);
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

size_t Varint::writeToStream(std::FILE* stream, uint32_t value) {
  byte b0, b1, b2, b3;
  if (value < 0x00000040) {  // 00000000 00000000 00000000 01000000
    b0 = value;
    mt::put(stream, b0);
    return 1;
  }
  if (value < 0x00004000) {  // 00000000 00000000 01000000 00000000
    b0 = (value >> 8) | 0x40;
    b1 = (value);
    mt::put(stream, b0);
    mt::put(stream, b1);
    return 2;
  }
  if (value < 0x00400000) {  // 00000000 01000000 00000000 00000000
    b0 = (value >> 16) | 0x80;
    b1 = (value >> 8);
    b2 = (value);
    mt::put(stream, b0);
    mt::put(stream, b1);
    mt::put(stream, b2);
    return 3;
  }
  if (value < 0x40000000) {  // 01000000 00000000 00000000 00000000
    b0 = (value >> 24) | 0xC0;
    b1 = (value >> 16);
    b2 = (value >> 8);
    b3 = (value);
    mt::put(stream, b0);
    mt::put(stream, b1);
    mt::put(stream, b2);
    mt::put(stream, b3);
    return 4;
  }
  mt::fail("Varint::writeToStream() failed. Too big value: %d", value);
  return 0;
}

void Varint::setFlagInBuffer(byte* buffer, bool flag) {
  flag ? (*buffer |= 0x20) : (*buffer &= 0xDF);
}

}  // namespace internal
}  // namespace multimap
