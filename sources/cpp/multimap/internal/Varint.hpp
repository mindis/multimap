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

#ifndef MULTIMAP_VARINT_HPP
#define MULTIMAP_VARINT_HPP

#include <cstdint>
#include "multimap/Bytes.hpp"

namespace multimap {
namespace internal {

struct Varint {
  static const std::uint32_t kMinValueStoredIn8Bits;
  static const std::uint32_t kMaxValueStoredIn8Bits;

  static const std::uint32_t kMinValueStoredIn16Bits;
  static const std::uint32_t kMaxValueStoredIn16Bits;

  static const std::uint32_t kMinValueStoredIn24Bits;
  static const std::uint32_t kMaxValueStoredIn24Bits;

  static const std::uint32_t kMinValueStoredIn32Bits;
  static const std::uint32_t kMaxValueStoredIn32Bits;

  static std::size_t ReadUint32(const byte* source, std::uint32_t* target);

  // Precondition: target has room for at least 4 bytes.
  static std::size_t WriteUint32(std::uint32_t source, byte* target);

  Varint() = delete;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_VARINT_HPP
