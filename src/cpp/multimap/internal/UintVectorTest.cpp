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

#include <type_traits>
#include "multimap/internal/UintVector.hpp"
#include "multimap/thirdparty/mt/assert.hpp"
#include "gmock/gmock.h"

namespace multimap {
namespace internal {

using testing::ElementsAreArray;

TEST(UintVector, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<UintVector>::value);
}

TEST(UintVector, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<UintVector>::value);
  ASSERT_FALSE(std::is_copy_assignable<UintVector>::value);
}

TEST(UintVector, IsMoveConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_move_constructible<UintVector>::value);
  ASSERT_TRUE(std::is_move_assignable<UintVector>::value);
}

TEST(UintVector, DefaultConstructedHasProperState) {
  ASSERT_TRUE(UintVector().unpack().empty());
  ASSERT_TRUE(UintVector().empty());
}

TEST(UintVector, AddAndUnpackMaxValue) {
  UintVector vector;
  vector.add(-1);
  ASSERT_EQ(vector.unpack().front(), -1);
}

TEST(UintVector, AddIncreasingValuesAndUnpack) {
  const uint32_t values[] = {0, 1, 10, 1000, 10000000, 100000000};
  UintVector vector;
  for (uint32_t value : values) {
    vector.add(value);
  }
  ASSERT_THAT(vector.unpack(), ElementsAreArray(values));
}

TEST(UintVector, AddDecreasingValuesAndThrow) {
  UintVector vector;
  const uint32_t values[] = {100000000, 10000000};
  vector.add(values[0]);
  ASSERT_THROW(vector.add(values[1]), mt::AssertionError);
}

class UintVectorWithParam : public testing::TestWithParam<uint32_t> {};

TEST_P(UintVectorWithParam, AddValueAndUnpack) {
  UintVector vector;
  vector.add(GetParam());
  ASSERT_EQ(vector.unpack().size(), 1);
  ASSERT_EQ(vector.unpack().front(), GetParam());
}

INSTANTIATE_TEST_CASE_P(Parameterized, UintVectorWithParam,
                        testing::Values(0, 1, 10, 1000, 10000000, 1000000000));

}  // namespace internal
}  // namespace multimap
