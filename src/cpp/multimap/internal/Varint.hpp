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

#ifndef MULTIMAP_INTERNAL_VARINT_HPP_INCLUDED
#define MULTIMAP_INTERNAL_VARINT_HPP_INCLUDED

#include <cstdint>

namespace multimap {
namespace internal {

struct Varint {

  struct Limits {

    static const std::uint32_t N1_MIN_UINT;
    static const std::uint32_t N1_MIN_UINT_WITH_FLAG;
    static const std::uint32_t N1_MAX_UINT;
    static const std::uint32_t N1_MAX_UINT_WITH_FLAG;

    static const std::uint32_t N2_MIN_UINT;
    static const std::uint32_t N2_MIN_UINT_WITH_FLAG;
    static const std::uint32_t N2_MAX_UINT;
    static const std::uint32_t N2_MAX_UINT_WITH_FLAG;

    static const std::uint32_t N3_MIN_UINT;
    static const std::uint32_t N3_MIN_UINT_WITH_FLAG;
    static const std::uint32_t N3_MAX_UINT;
    static const std::uint32_t N3_MAX_UINT_WITH_FLAG;

    static const std::uint32_t N4_MIN_UINT;
    static const std::uint32_t N4_MIN_UINT_WITH_FLAG;
    static const std::uint32_t N4_MAX_UINT;
    static const std::uint32_t N4_MAX_UINT_WITH_FLAG;

    static const std::uint32_t N5_MIN_UINT;
    static const std::uint32_t N5_MIN_UINT_WITH_FLAG;
    static const std::uint32_t N5_MAX_UINT;
    static const std::uint32_t N5_MAX_UINT_WITH_FLAG;

    Limits() = delete;
  };

  static std::size_t readUint(const char* buffer, std::size_t size,
                              std::uint32_t* value);

  static std::size_t readUintWithFlag(const char* buffer, std::size_t size,
                                      std::uint32_t* value, bool* flag);

  static std::size_t writeUint(std::uint32_t value, char* buffer,
                               std::size_t size);

  static std::size_t writeUintWithFlag(std::uint32_t value, bool flag,
                                       char* buffer, std::size_t size);

  static bool writeFlag(bool flag, char* buffer, std::size_t size);

  Varint() = delete;
};

} // namespace internal
} // namespace multimap

#endif // MULTIMAP_INTERNAL_VARINT_HPP_INCLUDED
