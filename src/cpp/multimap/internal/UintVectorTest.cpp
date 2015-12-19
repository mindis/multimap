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
#include "gmock/gmock.h"
#include "multimap/internal/UintVector.hpp"
#include "multimap/internal/Varint.hpp"

namespace multimap {
namespace internal {

using testing::ElementsAreArray;

TEST(UintVectorTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<UintVector>::value);
}

TEST(UintVectorTest, IsCopyConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_copy_constructible<UintVector>::value);
  ASSERT_TRUE(std::is_copy_assignable<UintVector>::value);
}

TEST(UintVectorTest, IsMoveConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_move_constructible<UintVector>::value);
  ASSERT_TRUE(std::is_move_assignable<UintVector>::value);
}

TEST(UintVectorTest, DefaultConstructedHasProperState) {
  ASSERT_TRUE(UintVector().unpack().empty());
  ASSERT_TRUE(UintVector().empty());
}

class UintVectorTestWithParam : public testing::TestWithParam<uint32_t> {};

TEST_P(UintVectorTestWithParam, AddValueAndUnpack) {
  UintVector vector;
  vector.add(GetParam());
  ASSERT_EQ(vector.unpack().size(), 1);
  ASSERT_EQ(vector.unpack().front(), GetParam());
}

INSTANTIATE_TEST_CASE_P(Parameterized, UintVectorTestWithParam,
                        testing::Values(0, 1, 10, 1000, 10000000,
                                        Varint::Limits::MAX_N4));

TEST(UintVectorTest, TryToAddTooLargeValue) {
  UintVector vector;
  ASSERT_FALSE(vector.add(Varint::Limits::MAX_N4 + 1));
}

TEST(UintVectorTest, AddIncreasingValuesAndUnpack) {
  UintVector vector;
  uint32_t values[] = {0, 1, 10, 1000, 10000000, Varint::Limits::MAX_N4};
  for (auto value : values) {
    vector.add(value);
  }
  ASSERT_THAT(vector.unpack(), ElementsAreArray(values));
}

TEST(UintVectorTest, AddDecreasingValuesAndThrow) {
  UintVector vector;
  uint32_t values[] = {Varint::Limits::MAX_N4, 10000000};
  vector.add(values[0]);
  ASSERT_THROW(vector.add(values[1]), mt::AssertionError);
}

}  // namespace internal
}  // namespace multimap
