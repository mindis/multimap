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

#ifndef MULTIMAP_INTERNAL_VARINT_HPP
#define MULTIMAP_INTERNAL_VARINT_HPP

#include <cstdint>
#include "multimap/Bytes.hpp"

namespace multimap {
namespace internal {

struct Varint {
  static std::size_t ReadUint32(const byte* source, std::uint32_t* target);

  // Precondition: target has room for at least 4 bytes.
  static std::size_t WriteUint32(std::uint32_t source, byte* target);

  static constexpr std::uint32_t min_value_1_byte() { return 0; }
  static constexpr std::uint32_t max_value_1_byte() { return (1 << 6) - 1; }
  static constexpr std::uint32_t min_value_2_bytes() { return (1 << 6); }
  static constexpr std::uint32_t max_value_2_bytes() { return (1 << 14) - 1; }
  static constexpr std::uint32_t min_value_3_bytes() { return (1 << 14); }
  static constexpr std::uint32_t max_value_3_bytes() { return (1 << 22) - 1; }
  static constexpr std::uint32_t min_value_4_bytes() { return (1 << 22); }
  static constexpr std::uint32_t max_value_4_bytes() { return (1 << 30) - 1; }

  Varint() = delete;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_VARINT_HPP
