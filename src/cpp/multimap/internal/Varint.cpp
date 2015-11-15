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

#include "multimap/internal/Varint.hpp"

#include "multimap/thirdparty/mt.hpp"

namespace multimap {
namespace internal {

const std::uint32_t Varint::Limits::N1_MIN_UINT = 0;
const std::uint32_t Varint::Limits::N1_MIN_UINT_WITH_FLAG = 0;
const std::uint32_t Varint::Limits::N1_MAX_UINT = (1 << 7) - 1;
const std::uint32_t Varint::Limits::N1_MAX_UINT_WITH_FLAG = (1 << 6) - 1;

const std::uint32_t Varint::Limits::N2_MIN_UINT = (1 << 7);
const std::uint32_t Varint::Limits::N2_MIN_UINT_WITH_FLAG = (1 << 6);
const std::uint32_t Varint::Limits::N2_MAX_UINT = (1 << 14) - 1;
const std::uint32_t Varint::Limits::N2_MAX_UINT_WITH_FLAG = (1 << 13) - 1;

const std::uint32_t Varint::Limits::N3_MIN_UINT = (1 << 14);
const std::uint32_t Varint::Limits::N3_MIN_UINT_WITH_FLAG = (1 << 13);
const std::uint32_t Varint::Limits::N3_MAX_UINT = (1 << 21) - 1;
const std::uint32_t Varint::Limits::N3_MAX_UINT_WITH_FLAG = (1 << 20) - 1;

const std::uint32_t Varint::Limits::N4_MIN_UINT = (1 << 21);
const std::uint32_t Varint::Limits::N4_MIN_UINT_WITH_FLAG = (1 << 20);
const std::uint32_t Varint::Limits::N4_MAX_UINT = (1 << 28) - 1;
const std::uint32_t Varint::Limits::N4_MAX_UINT_WITH_FLAG = (1 << 27) - 1;

const std::uint32_t Varint::Limits::N5_MIN_UINT = (1 << 28);
const std::uint32_t Varint::Limits::N5_MIN_UINT_WITH_FLAG = (1 << 27);

const std::uint32_t Varint::Limits::N5_MAX_UINT =
    std::numeric_limits<std::uint32_t>::max();

const std::uint32_t Varint::Limits::N5_MAX_UINT_WITH_FLAG =
    std::numeric_limits<std::uint32_t>::max();

std::size_t Varint::readUint(const char* buffer, std::size_t size,
                             std::uint32_t* value) {
  MT_REQUIRE_NOT_NULL(buffer);
  MT_REQUIRE_NOT_NULL(value);

  const std::uint8_t* ptr = reinterpret_cast<const std::uint8_t*>(buffer);
  if (size == 0) return 0;
  *value = *ptr & 0x7F;
  --size;

  auto shift = 7;
  while (*ptr++ & 0x80) {
    if (size == 0) return 0;
    *value += (*ptr & 0x7F) << shift;
    shift += 7;
    --size;
  }
  return ptr - reinterpret_cast<const std::uint8_t*>(buffer);
}

// std::size_t Varint::readUint(const char* buffer, std::size_t size,
//                             std::uint32_t* value) {
//  MT_REQUIRE_NOT_NULL(buffer);
//  MT_REQUIRE_NOT_NULL(value);

//  if (size > 0) {
//    const std::uint8_t* ptr = reinterpret_cast<const std::uint8_t*>(buffer);
//    const int length = (ptr[0] & 0xC0); // 11000000
//    switch (length) {
//      case 0x00: // 00000000
//        *value = ptr[0];
//        return 1;
//      case 0x40: // 01000000
//        if (size < 2) {
//          break;
//        }
//        *value = (ptr[0] & 0x3F) << 8;
//        *value += ptr[1];
//        return 2;
//      case 0x80: // 10000000
//        if (size < 3) {
//          break;
//        }
//        *value = (ptr[0] & 0x3F) << 16;
//        *value += ptr[1] << 8;
//        *value += ptr[2];
//        return 3;
//      case 0xC0: // 11000000
//        if (size < 4) {
//          break;
//        }
//        *value = (ptr[0] & 0x3F) << 24;
//        *value += ptr[1] << 16;
//        *value += ptr[2] << 8;
//        *value += ptr[3];
//        return 4;
//      default:
//        mt::throwRuntimeError(mt::Messages::FATAL_ERROR);
//    }
//  }
//  return 0;
//}

std::size_t Varint::readUintWithFlag(const char* buffer, std::size_t size,
                                     std::uint32_t* value, bool* flag) {
  MT_REQUIRE_NOT_NULL(buffer);
  MT_REQUIRE_NOT_NULL(value);
  MT_REQUIRE_NOT_NULL(flag);

  const std::uint8_t* ptr = reinterpret_cast<const std::uint8_t*>(buffer);
  if (size == 0) return 0;
  *value = *ptr & 0x3F;
  *flag = *ptr & 0x40;
  --size;

  auto shift = 6;
  while (*ptr++ & 0x80) {
    if (size == 0) return 0;
    *value += (*ptr & 0x7F) << shift;
    shift += 7;
    --size;
  }
  return ptr - reinterpret_cast<const std::uint8_t*>(buffer);
}

// std::size_t Varint::readUintWithFlag(const char* buffer, std::size_t size,
//                                     std::uint32_t* value, bool* flag) {
//  MT_REQUIRE_NOT_NULL(buffer);
//  MT_REQUIRE_NOT_NULL(value);
//  MT_REQUIRE_NOT_NULL(flag);

//  if (size > 0) {
//    const std::uint8_t* ptr = reinterpret_cast<const std::uint8_t*>(buffer);
//    *flag = (ptr[0] & 0x80);            // 10000000
//    const int length = (ptr[0] & 0x60); // 01100000
//    switch (length) {
//      case 0x00: // 00000000
//        *value = (ptr[0] & 0x1F);
//        return 1;
//      case 0x20: // 00100000
//        if (size < 2) {
//          break;
//        }
//        *value = (ptr[0] & 0x1F) << 8;
//        *value += ptr[1];
//        return 2;
//      case 0x40: // 01000000
//        if (size < 3) {
//          break;
//        }
//        *value = (ptr[0] & 0x1F) << 16;
//        *value += ptr[1] << 8;
//        *value += ptr[2];
//        return 3;
//      case 0x60: // 01100000
//        if (size < 4) {
//          break;
//        }
//        *value = (ptr[0] & 0x1F) << 24;
//        *value += ptr[1] << 16;
//        *value += ptr[2] << 8;
//        *value += ptr[3];
//        return 4;
//      default:
//        mt::throwRuntimeError(mt::Messages::FATAL_ERROR);
//    }
//  }
//  return 0;
//}

std::size_t Varint::writeUint(std::uint32_t value, char* buffer,
                              std::size_t size) {
  MT_REQUIRE_NOT_NULL(buffer);

  std::uint8_t* ptr = reinterpret_cast<std::uint8_t*>(buffer);
  while (value > 0x7F) {
    if (size == 0) return 0;
    *ptr++ = 0x80 | (value & 0x7f);
    value >>= 7;
    --size;
  }
  if (size == 0) return 0;
  *ptr++ = value;
  return ptr - reinterpret_cast<std::uint8_t*>(buffer);
}

// std::size_t Varint::writeUint(std::uint32_t value, char* buffer,
//                              std::size_t size) {
//  MT_REQUIRE_NOT_NULL(buffer);

//  std::uint8_t* ptr = reinterpret_cast<std::uint8_t*>(buffer);
//  if (value < 0x00000040) { // 00000000 00000000 00000000 01000000
//    if (size < 1) {
//      goto too_few_bytes;
//    }
//    ptr[0] = value;
//    return 1;
//  }
//  if (value < 0x00004000) { // 00000000 00000000 01000000 00000000
//    if (size < 2) {
//      goto too_few_bytes;
//    }
//    ptr[0] = (value >> 8) | 0x40;
//    ptr[1] = (value);
//    return 2;
//  }
//  if (value < 0x00400000) { // 00000000 01000000 00000000 00000000
//    if (size < 3) {
//      goto too_few_bytes;
//    }
//    ptr[0] = (value >> 16) | 0x80;
//    ptr[1] = (value >> 8);
//    ptr[2] = (value);
//    return 3;
//  }
//  if (value < 0x40000000) { // 01000000 00000000 00000000 00000000
//    if (size < 4) {
//      goto too_few_bytes;
//    }
//    ptr[0] = (value >> 24) | 0xC0;
//    ptr[1] = (value >> 16);
//    ptr[2] = (value >> 8);
//    ptr[3] = (value);
//    return 4;
//  }
// too_few_bytes:
//  return 0;
//}

std::size_t Varint::writeUintWithFlag(std::uint32_t value, bool flag,
                                      char* buffer, std::size_t size) {
  MT_REQUIRE_NOT_NULL(buffer);

  std::uint8_t* ptr = reinterpret_cast<std::uint8_t*>(buffer);
  if (size == 0) return 0;
  if (value > 0x3F) {
    *ptr++ = (flag ? 0xC0 : 0x80) | (value & 0x3F);
    value >>= 6;
    --size;
    while (value > 0x7f) {
      if (size == 0) return 0;
      *ptr++ = 0x80 | (value & 0x7f);
      value >>= 7;
      --size;
    }
    if (size == 0) return 0;
    *ptr++ = value;
  } else {
    *ptr++ = flag ? (0x40 | value) : value;
  }
  return ptr - reinterpret_cast<std::uint8_t*>(buffer);
}

// std::size_t Varint::writeUintWithFlag(std::uint32_t value, bool flag,
//                                      char* buffer, std::size_t size) {
//  MT_REQUIRE_NOT_NULL(buffer);

//  std::uint8_t* ptr = reinterpret_cast<std::uint8_t*>(buffer);
//  if (value < 0x00000020) { // 00000000 00000000 00000000 00100000
//    if (size < 1) {
//      goto too_few_bytes;
//    }
//    ptr[0] = value | (flag ? 0x80 : 0x00);
//    return 1;
//  }
//  if (value < 0x00002000) { // 00000000 00000000 00100000 00000000
//    if (size < 2) {
//      goto too_few_bytes;
//    }
//    ptr[0] = (value >> 8) | (flag ? 0xA0 : 0x20);
//    ptr[1] = (value);
//    return 2;
//  }
//  if (value < 0x00200000) { // 00000000 00100000 00000000 00000000
//    if (size < 3) {
//      goto too_few_bytes;
//    }
//    ptr[0] = (value >> 16) | (flag ? 0xC0 : 0x40);
//    ptr[1] = (value >> 8);
//    ptr[2] = (value);
//    return 3;
//  }
//  if (value < 0x20000000) { // 00100000 00000000 00000000 00000000
//    if (size < 4) {
//      goto too_few_bytes;
//    }
//    ptr[0] = (value >> 24) | (flag ? 0xE0 : 0x60);
//    ptr[1] = (value >> 16);
//    ptr[2] = (value >> 8);
//    ptr[3] = (value);
//    return 4;
//  }
// too_few_bytes:
//  return 0;
//}

bool Varint::writeFlag(bool flag, char* buffer, std::size_t size) {
  MT_REQUIRE_NOT_NULL(buffer);

  if (size > 0) {
    if (flag) {
      *reinterpret_cast<std::uint8_t*>(buffer) |= 0x40;
    } else {
      *reinterpret_cast<std::uint8_t*>(buffer) &= 0xBF;
    }
    return true;
  }
  return false;
}

// bool Varint::writeFlag(bool flag, char* buffer, std::size_t size) {
//  MT_REQUIRE_NOT_NULL(buffer);

//  if (size > 0) {
//    if (flag) {
//      *reinterpret_cast<std::uint8_t*>(buffer) |= 0x80;
//    } else {
//      *reinterpret_cast<std::uint8_t*>(buffer) &= 0x7F;
//    }
//    return true;
//  }
//  return false;
//}

} // namespace internal
} // namespace multimap
