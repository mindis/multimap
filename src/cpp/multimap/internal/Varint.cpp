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

uint32_t Varint::readUintFromBuffer(const char* buffer, size_t len,
                                    uint32_t* value) {
  MT_REQUIRE_NOT_NULL(buffer);
  MT_REQUIRE_NOT_NULL(value);

  if (len > 0) {
    const std::uint8_t* ptr = reinterpret_cast<const std::uint8_t*>(buffer);
    const int length = (ptr[0] & 0xC0);  // 11000000
    switch (length) {
      case 0x00:  // 00000000
        *value = ptr[0];
        return 1;
      case 0x40:  // 01000000
        if (len < 2) {
          break;
        }
        *value = (ptr[0] & 0x3F) << 8;
        *value += ptr[1];
        return 2;
      case 0x80:  // 10000000
        if (len < 3) {
          break;
        }
        *value = (ptr[0] & 0x3F) << 16;
        *value += ptr[1] << 8;
        *value += ptr[2];
        return 3;
      case 0xC0:  // 11000000
        if (len < 4) {
          break;
        }
        *value = (ptr[0] & 0x3F) << 24;
        *value += ptr[1] << 16;
        *value += ptr[2] << 8;
        *value += ptr[3];
        return 4;
      default:
        mt::fail("Invalid varint encoding");
    }
  }
  return 0;
}

uint32_t Varint::readUintWithFlagFromBuffer(const char* buffer, size_t len,
                                            uint32_t* value, bool* flag) {
  MT_REQUIRE_NOT_NULL(buffer);
  MT_REQUIRE_NOT_NULL(value);
  MT_REQUIRE_NOT_NULL(flag);

  if (len > 0) {
    const std::uint8_t* ptr = reinterpret_cast<const std::uint8_t*>(buffer);
    const int length = (ptr[0] & 0xC0);  // 11000000
    *flag = (ptr[0] & 0x20);             // 00100000
    switch (length) {
      case 0x00:  // 00000000
        *value = (ptr[0] & 0x1F);
        return 1;
      case 0x40:  // 01000000
        if (len < 2) {
          break;
        }
        *value = (ptr[0] & 0x1F) << 8;
        *value += ptr[1];
        return 2;
      case 0x80:  // 10000000
        if (len < 3) {
          break;
        }
        *value = (ptr[0] & 0x1F) << 16;
        *value += ptr[1] << 8;
        *value += ptr[2];
        return 3;
      case 0xC0:  // 11000000
        if (len < 4) {
          break;
        }
        *value = (ptr[0] & 0x1F) << 24;
        *value += ptr[1] << 16;
        *value += ptr[2] << 8;
        *value += ptr[3];
        return 4;
      default:
        mt::fail("Invalid varint with flag encoding");
    }
  }
  return 0;
}

uint32_t Varint::writeUintToBuffer(uint32_t value, char* buffer, size_t len) {
  MT_REQUIRE_NOT_NULL(buffer);

  std::uint8_t* ptr = reinterpret_cast<std::uint8_t*>(buffer);
  if (value < 0x00000040) {  // 00000000 00000000 00000000 01000000
    if (len < 1) {
      goto too_few_bytes;
    }
    ptr[0] = value;
    return 1;
  }
  if (value < 0x00004000) {  // 00000000 00000000 01000000 00000000
    if (len < 2) {
      goto too_few_bytes;
    }
    ptr[0] = (value >> 8) | 0x40;
    ptr[1] = (value);
    return 2;
  }
  if (value < 0x00400000) {  // 00000000 01000000 00000000 00000000
    if (len < 3) {
      goto too_few_bytes;
    }
    ptr[0] = (value >> 16) | 0x80;
    ptr[1] = (value >> 8);
    ptr[2] = (value);
    return 3;
  }
  if (value < 0x40000000) {  // 01000000 00000000 00000000 00000000
    if (len < 4) {
      goto too_few_bytes;
    }
    ptr[0] = (value >> 24) | 0xC0;
    ptr[1] = (value >> 16);
    ptr[2] = (value >> 8);
    ptr[3] = (value);
    return 4;
  }
  mt::fail("Cannot encode too big value: %d", value);
too_few_bytes:
  return 0;
}

uint32_t Varint::writeUintWithFlagToBuffer(uint32_t value, bool flag,
                                           char* buffer, size_t len) {
  MT_REQUIRE_NOT_NULL(buffer);

  std::uint8_t* ptr = reinterpret_cast<std::uint8_t*>(buffer);
  if (value < 0x00000020) {  // 00000000 00000000 00000000 00100000
    if (len < 1) {
      goto too_few_bytes;
    }
    ptr[0] = value | (flag ? 0x20 : 0x00);
    return 1;
  }
  if (value < 0x00002000) {  // 00000000 00000000 00100000 00000000
    if (len < 2) {
      goto too_few_bytes;
    }
    ptr[0] = (value >> 8) | (flag ? 0x60 : 0x40);
    ptr[1] = (value);
    return 2;
  }
  if (value < 0x00200000) {  // 00000000 00100000 00000000 00000000
    if (len < 3) {
      goto too_few_bytes;
    }
    ptr[0] = (value >> 16) | (flag ? 0xA0 : 0x80);
    ptr[1] = (value >> 8);
    ptr[2] = (value);
    return 3;
  }
  if (value < 0x20000000) {  // 00100000 00000000 00000000 00000000
    if (len < 4) {
      goto too_few_bytes;
    }
    ptr[0] = (value >> 24) | (flag ? 0xE0 : 0xC0);
    ptr[1] = (value >> 16);
    ptr[2] = (value >> 8);
    ptr[3] = (value);
    return 4;
  }
  mt::fail("Cannot encode too big value: %d", value);
too_few_bytes:
  return 0;
}

void Varint::writeFlagToBuffer(bool flag, char* buffer, size_t len) {
  MT_REQUIRE_NOT_NULL(buffer);
  MT_REQUIRE_NOT_ZERO(len);

  if (flag) {
    *reinterpret_cast<std::uint8_t*>(buffer) |= 0x20;
  } else {
    *reinterpret_cast<std::uint8_t*>(buffer) &= 0xDF;
  }
}

}  // namespace internal
}  // namespace multimap
