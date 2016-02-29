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

uint32_t Varint::readUintFromBuffer(const char* buf, size_t len,
                                    uint32_t* val) {
  MT_REQUIRE_NOT_NULL(buf);
  MT_REQUIRE_NOT_NULL(val);

  if (len > 0) {
    const std::uint8_t* ptr = reinterpret_cast<const std::uint8_t*>(buf);
    const int length = (ptr[0] & 0xC0);  // 11000000
    switch (length) {
      case 0x00:  // 00000000
        *val = ptr[0];
        return 1;
      case 0x40:  // 01000000
        if (len < 2) {
          break;
        }
        *val = (ptr[0] & 0x3F) << 8;
        *val += ptr[1];
        return 2;
      case 0x80:  // 10000000
        if (len < 3) {
          break;
        }
        *val = (ptr[0] & 0x3F) << 16;
        *val += ptr[1] << 8;
        *val += ptr[2];
        return 3;
      case 0xC0:  // 11000000
        if (len < 4) {
          break;
        }
        *val = (ptr[0] & 0x3F) << 24;
        *val += ptr[1] << 16;
        *val += ptr[2] << 8;
        *val += ptr[3];
        return 4;
      default:
        mt::fail("Invalid varint encoding");
    }
  }
  return 0;
}

uint32_t readUintFromStream(std::FILE* stream, uint32_t* val) {
  MT_REQUIRE_NOT_NULL(stream);
  MT_REQUIRE_NOT_NULL(val);

  int ch = std::fgetc(stream);
  if (ch == EOF) return 0;

  uint8_t b0 = ch;
  uint8_t b1, b2, b3;
  switch (b0 & 0xC0) {
    case 0x00:  // 00000000
      *val = b0;
      return 1;
    case 0x40:  // 01000000
      ch = std::fgetc(stream);
      if (ch == EOF) return 0;
      b1 = ch;
      *val = (b0 & 0x3F) << 8;
      *val += b1;
      return 2;
    case 0x80:  // 10000000
      ch = std::fgetc(stream);
      if (ch == EOF) return 0;
      b1 = ch;
      ch = std::fgetc(stream);
      if (ch == EOF) return 0;
      b2 = ch;
      *val = (b0 & 0x3F) << 16;
      *val += b1 << 8;
      *val += b2;
      return 3;
    case 0xC0:  // 11000000
      ch = std::fgetc(stream);
      if (ch == EOF) return 0;
      b1 = ch;
      ch = std::fgetc(stream);
      if (ch == EOF) return 0;
      b2 = ch;
      ch = std::fgetc(stream);
      if (ch == EOF) return 0;
      b3 = ch;
      *val = (b0 & 0x3F) << 24;
      *val += b1 << 16;
      *val += b2 << 8;
      *val += b3;
      return 4;
    default:
      mt::fail("Invalid varint encoding");
  }
  return 0;
}

uint32_t Varint::readUintWithFlagFromBuffer(const char* buf, size_t len,
                                            uint32_t* val, bool* flag) {
  MT_REQUIRE_NOT_NULL(buf);
  MT_REQUIRE_NOT_NULL(val);
  MT_REQUIRE_NOT_NULL(flag);

  if (len > 0) {
    const std::uint8_t* ptr = reinterpret_cast<const std::uint8_t*>(buf);
    const int length = (ptr[0] & 0xC0);  // 11000000
    *flag = (ptr[0] & 0x20);             // 00100000
    switch (length) {
      case 0x00:  // 00000000
        *val = (ptr[0] & 0x1F);
        return 1;
      case 0x40:  // 01000000
        if (len < 2) {
          break;
        }
        *val = (ptr[0] & 0x1F) << 8;
        *val += ptr[1];
        return 2;
      case 0x80:  // 10000000
        if (len < 3) {
          break;
        }
        *val = (ptr[0] & 0x1F) << 16;
        *val += ptr[1] << 8;
        *val += ptr[2];
        return 3;
      case 0xC0:  // 11000000
        if (len < 4) {
          break;
        }
        *val = (ptr[0] & 0x1F) << 24;
        *val += ptr[1] << 16;
        *val += ptr[2] << 8;
        *val += ptr[3];
        return 4;
      default:
        mt::fail("Invalid varint with flag encoding");
    }
  }
  return 0;
}

uint32_t Varint::writeUintToBuffer(char* buf, size_t len, uint32_t val) {
  MT_REQUIRE_NOT_NULL(buf);

  std::uint8_t* ptr = reinterpret_cast<std::uint8_t*>(buf);
  if (val < 0x00000040) {  // 00000000 00000000 00000000 01000000
    if (len < 1) {
      goto too_few_bytes;
    }
    ptr[0] = val;
    return 1;
  }
  if (val < 0x00004000) {  // 00000000 00000000 01000000 00000000
    if (len < 2) {
      goto too_few_bytes;
    }
    ptr[0] = (val >> 8) | 0x40;
    ptr[1] = (val);
    return 2;
  }
  if (val < 0x00400000) {  // 00000000 01000000 00000000 00000000
    if (len < 3) {
      goto too_few_bytes;
    }
    ptr[0] = (val >> 16) | 0x80;
    ptr[1] = (val >> 8);
    ptr[2] = (val);
    return 3;
  }
  if (val < 0x40000000) {  // 01000000 00000000 00000000 00000000
    if (len < 4) {
      goto too_few_bytes;
    }
    ptr[0] = (val >> 24) | 0xC0;
    ptr[1] = (val >> 16);
    ptr[2] = (val >> 8);
    ptr[3] = (val);
    return 4;
  }
  mt::fail("Cannot encode too big value: %d", val);
too_few_bytes:
  return 0;
}

uint32_t writeUintToStream(std::FILE* stream, uint32_t val) {
  MT_REQUIRE_NOT_NULL(stream);

  uint8_t b0, b1, b2, b3;
  if (val < 0x00000040) {  // 00000000 00000000 00000000 01000000
    b0 = val;
    if (std::fputc(b0, stream) == EOF) return 0;
    return 1;
  }
  if (val < 0x00004000) {  // 00000000 00000000 01000000 00000000
    b0 = (val >> 8) | 0x40;
    b1 = (val);
    if (std::fputc(b0, stream) == EOF) return 0;
    if (std::fputc(b1, stream) == EOF) return 0;
    return 2;
  }
  if (val < 0x00400000) {  // 00000000 01000000 00000000 00000000
    b0 = (val >> 16) | 0x80;
    b1 = (val >> 8);
    b2 = (val);
    if (std::fputc(b0, stream) == EOF) return 0;
    if (std::fputc(b1, stream) == EOF) return 0;
    if (std::fputc(b2, stream) == EOF) return 0;
    return 3;
  }
  if (val < 0x40000000) {  // 01000000 00000000 00000000 00000000
    b0 = (val >> 24) | 0xC0;
    b1 = (val >> 16);
    b2 = (val >> 8);
    b3 = (val);
    if (std::fputc(b0, stream) == EOF) return 0;
    if (std::fputc(b1, stream) == EOF) return 0;
    if (std::fputc(b2, stream) == EOF) return 0;
    if (std::fputc(b3, stream) == EOF) return 0;
    return 4;
  }
  mt::fail("Cannot encode too big value: %d", val);
  return 0;
}

uint32_t Varint::writeUintWithFlagToBuffer(char* buf, size_t len, uint32_t val,
                                           bool flag) {
  MT_REQUIRE_NOT_NULL(buf);

  std::uint8_t* ptr = reinterpret_cast<std::uint8_t*>(buf);
  if (val < 0x00000020) {  // 00000000 00000000 00000000 00100000
    if (len < 1) {
      goto too_few_bytes;
    }
    ptr[0] = val | (flag ? 0x20 : 0x00);
    return 1;
  }
  if (val < 0x00002000) {  // 00000000 00000000 00100000 00000000
    if (len < 2) {
      goto too_few_bytes;
    }
    ptr[0] = (val >> 8) | (flag ? 0x60 : 0x40);
    ptr[1] = (val);
    return 2;
  }
  if (val < 0x00200000) {  // 00000000 00100000 00000000 00000000
    if (len < 3) {
      goto too_few_bytes;
    }
    ptr[0] = (val >> 16) | (flag ? 0xA0 : 0x80);
    ptr[1] = (val >> 8);
    ptr[2] = (val);
    return 3;
  }
  if (val < 0x20000000) {  // 00100000 00000000 00000000 00000000
    if (len < 4) {
      goto too_few_bytes;
    }
    ptr[0] = (val >> 24) | (flag ? 0xE0 : 0xC0);
    ptr[1] = (val >> 16);
    ptr[2] = (val >> 8);
    ptr[3] = (val);
    return 4;
  }
  mt::fail("Cannot encode too big value: %d", val);
too_few_bytes:
  return 0;
}

void Varint::writeFlagToBuffer(char* buf, size_t len, bool flag) {
  MT_REQUIRE_NOT_NULL(buf);
  MT_REQUIRE_NOT_ZERO(len);

  if (flag) {
    *reinterpret_cast<std::uint8_t*>(buf) |= 0x20;
  } else {
    *reinterpret_cast<std::uint8_t*>(buf) &= 0xDF;
  }
}

}  // namespace internal
}  // namespace multimap
