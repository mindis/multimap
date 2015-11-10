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

#include <type_traits>
#include <gmock/gmock.h>
#include "multimap/internal/UintVector.hpp"

namespace multimap {
namespace internal {

using testing::Eq;
using testing::ElementsAreArray;

struct UintVectorTestParam : public testing::TestWithParam<std::uint32_t> {};

TEST(UintVectorTest, IsDefaultConstructible) {
  ASSERT_THAT(std::is_default_constructible<UintVector>::value, Eq(true));
}

TEST(UintVectorTest, DefaultConstructedHasProperState) {
  ASSERT_THAT(UintVector().unpack().empty(), Eq(true));
  ASSERT_THAT(UintVector().empty(), Eq(true));
}

TEST(UintVectorTest, IsCopyConstructibleAndAssignable) {
  ASSERT_THAT(std::is_copy_constructible<UintVector>::value, Eq(true));
  ASSERT_THAT(std::is_copy_assignable<UintVector>::value, Eq(true));
}

TEST(UintVectorTest, IsMoveConstructibleAndAssignable) {
  ASSERT_THAT(std::is_move_constructible<UintVector>::value, Eq(true));
  ASSERT_THAT(std::is_move_assignable<UintVector>::value, Eq(true));
}

TEST_P(UintVectorTestParam, AddValueAndUnpack) {
  UintVector vector;
  vector.add(GetParam());
  ASSERT_THAT(vector.unpack().size(), Eq(1));
  ASSERT_THAT(vector.unpack().front(), Eq(GetParam()));
}

INSTANTIATE_TEST_CASE_P(Parameterized, UintVectorTestParam,
                        testing::Values(0, 1, 10, 1000, 10000000,
                                        Varint::Limits::N4_MAX_UINT));

TEST(UintVectorTest, TryToAddTooLargeValue) {
  UintVector vector;
  ASSERT_FALSE(vector.add(Varint::Limits::N4_MAX_UINT + 1));
}

TEST(UintVectorTest, AddIncreasingValuesAndUnpack) {
  UintVector vector;
  std::uint32_t values[] = { 0,    1,        10,
                             1000, 10000000, Varint::Limits::N4_MAX_UINT };
  for (auto value : values) {
    vector.add(value);
  }
  ASSERT_THAT(vector.unpack(), ElementsAreArray(values));
}

TEST(UintVectorTest, AddDecreasingValuesAndThrow) {
  UintVector vector;
  std::uint32_t values[] = { Varint::Limits::N4_MAX_UINT, 10000000 };
  vector.add(values[0]);
  ASSERT_THROW(vector.add(values[1]), mt::AssertionError);
}

} // namespace internal
} // namespace multimap
