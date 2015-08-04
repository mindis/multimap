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

#include <cassert>
#include <cstring>
#include "multimap/internal/Check.hpp"

namespace multimap {
namespace internal {

std::size_t Varint::ReadUint32(const byte* source, std::uint32_t* target) {
  assert(source != nullptr);
  assert(target != nullptr);
  const auto length = *source & 0xC0;  // 11000000
  switch (length) {
    case 0x00:  // 00000000
      *target = *source;
      return 1;
    case 0x40:  // 01000000
      *target = (source[0] & 0x3F) << 8;
      *target += source[1];
      return 2;
    case 0x80:  // 10000000
      *target = (source[0] & 0x3F) << 16;
      *target += source[1] << 8;
      *target += source[2];
      return 3;
    case 0xC0:  // 11000000
      *target = (source[0] & 0x3F) << 24;
      *target += source[1] << 16;
      *target += source[2] << 8;
      *target += source[3];
      return 4;
    default:
      assert(false);
  }
}

std::size_t Varint::WriteUint32(std::uint32_t source, byte* target) {
  assert(target != nullptr);
  if (source < 0x00000040) {  // 00000000 00000000 00000000 01000000
    target[0] = static_cast<std::uint8_t>(source);
    return 1;
  }
  if (source < 0x00004000) {  // 00000000 00000000 01000000 00000000
    source |= 0x00004000;     // 00000000 00000000 01000000 00000000
    target[0] = static_cast<std::uint8_t>(source >> 8);
    target[1] = static_cast<std::uint8_t>(source);
    return 2;
  }
  if (source < 0x00400000) {  // 00000000 01000000 00000000 00000000
    source |= 0x00800000;     // 00000000 10000000 00000000 00000000
    target[0] = static_cast<std::uint8_t>(source >> 16);
    target[1] = static_cast<std::uint8_t>(source >> 8);
    target[2] = static_cast<std::uint8_t>(source);
    return 3;
  }
  if (source < 0x40000000) {  // 01000000 00000000 00000000 00000000
    source |= 0xC0000000;     // 11000000 00000000 00000000 00000000
    target[0] = static_cast<std::uint8_t>(source >> 24);
    target[1] = static_cast<std::uint8_t>(source >> 16);
    target[2] = static_cast<std::uint8_t>(source >> 8);
    target[3] = static_cast<std::uint8_t>(source);
    return 4;
  }
  Throw("Varint::WriteUint32 source is out of range");
  return 0;  // To suppress "no return value" warning.
}

}  // namespace internal
}  // namespace multimap
