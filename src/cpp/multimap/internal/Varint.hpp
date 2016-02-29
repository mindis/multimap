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

#ifndef MULTIMAP_INTERNAL_VARINT_HPP_INCLUDED
#define MULTIMAP_INTERNAL_VARINT_HPP_INCLUDED

#include <cstddef>
#include <cstdint>

namespace multimap {
namespace internal {

struct Varint {
  struct Limits {
    static const uint32_t MIN_N1;
    static const uint32_t MIN_N2;
    static const uint32_t MIN_N3;
    static const uint32_t MIN_N4;

    static const uint32_t MAX_N1;
    static const uint32_t MAX_N2;
    static const uint32_t MAX_N3;
    static const uint32_t MAX_N4;

    static const uint32_t MIN_N1_WITH_FLAG;
    static const uint32_t MIN_N2_WITH_FLAG;
    static const uint32_t MIN_N3_WITH_FLAG;
    static const uint32_t MIN_N4_WITH_FLAG;

    static const uint32_t MAX_N1_WITH_FLAG;
    static const uint32_t MAX_N2_WITH_FLAG;
    static const uint32_t MAX_N3_WITH_FLAG;
    static const uint32_t MAX_N4_WITH_FLAG;

    Limits() = delete;
  };

  static uint32_t readUintFromBuffer(const char* buf, size_t len,
                                     uint32_t* value);
  // Reads a 32-bit unsigned integer from `buffer` into `output`.
  // Returns the number of bytes read on success, otherwise zero.
  // Preconditions:
  //  * `buf` is not null
  //  * `value` is not null

  static uint32_t readUintWithFlagFromBuffer(const char* buf, size_t len,
                                             uint32_t* value, bool* flag);
  // Reads a 32-bit unsigned integer with flag from `buffer` into `value` and
  // `flag`. Returns the number of bytes read on success, otherwise zero.
  // Preconditions:
  //  * `buf` is not null
  //  * `value` is not null
  //  * `flag` is not null

  static uint32_t writeUintToBuffer(uint32_t value, char* buf, size_t len);
  // Writes a 32-bit unsigned integer into `buffer`.
  // Returns the number of bytes written on success, otherwise zero.
  // Preconditions:
  //  * `value` is not greater than `MAX_N4`
  //  * `buf` is not null

  static uint32_t writeUintWithFlagToBuffer(uint32_t value, bool flag,
                                            char* buf, size_t len);
  // Writes a 32-bit unsigned integer with flag into `buffer`.
  // Returns the number of bytes written on success, otherwise zero.
  // Preconditions:
  //  * `value` is not greater than `MAX_N4_WITH_FLAG`
  //  * `buf` is not null

  static void writeFlagToBuffer(bool flag, char* buf, size_t len);
  // Sets a flag in `buffer` which is expected to point to the first byte of a
  // valid varint encoding produced by a call to `writeUintWithFlagToBuffer()`.
  // Preconditions:
  //  * `buf` is not null
  //  * `len` is not zero

  Varint() = delete;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_VARINT_HPP_INCLUDED
