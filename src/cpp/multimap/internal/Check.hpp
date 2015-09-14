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

#ifndef MULTIMAP_INCLUDE_INTERNAL_CHECK_HPP
#define MULTIMAP_INCLUDE_INTERNAL_CHECK_HPP

#include <cstddef>

namespace multimap {
namespace internal {

enum class Arch { k32, k64 };

template <std::size_t SizeOfVoidPtr>
struct ToArch;

template <>
struct ToArch<4> {
  static const Arch value = Arch::k32;
};

template <>
struct ToArch<8> {
  static const Arch value = Arch::k64;
};

template <typename T, Arch>
struct ExpectedSizeOf;

template <typename T, std::size_t ExpectedSize32, std::size_t ExpectedSize64>
struct HasExpectedSize {
  static const bool is32arch = ToArch<sizeof(void*)>::value == Arch::k32;
  static const auto expected = is32arch ? ExpectedSize32 : ExpectedSize64;
  static const bool value = sizeof(T) == expected;
};

// Usage:
// static_assert(HasExpectedSize<List, 40, 40>::value,
//               "class List does not have expected size");

void Check(bool expression, const char* format, ...);

void Throw(const char* format, ...);

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INCLUDE_INTERNAL_CHECK_HPP
