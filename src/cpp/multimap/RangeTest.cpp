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
#include "gmock/gmock.h"
#include "multimap/Range.hpp"

namespace multimap {

TEST(RangeTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<Range>::value);
}

TEST(RangeTest, IsCopyConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_copy_constructible<Range>::value);
  ASSERT_TRUE(std::is_copy_assignable<Range>::value);
}

TEST(RangeTest, IsMoveConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_move_constructible<Range>::value);
  ASSERT_TRUE(std::is_move_assignable<Range>::value);
}

TEST(RangeTest, EqualityOperator) {
  ASSERT_TRUE(Range("abc") == Range("abc"));
  ASSERT_FALSE(Range("bc") == Range("abc"));
}

TEST(RangeTest, EqualityOperatorTakesCString) {
  ASSERT_TRUE(Range("abc") == "abc");
  ASSERT_FALSE(Range("bc") == "abc");
}

TEST(RangeTest, EqualityOperatorTakesStdString) {
  ASSERT_TRUE(Range("abc") == std::string("abc"));
  ASSERT_FALSE(Range("bc") == std::string("abc"));
}

TEST(RangeTest, InequalityOperator) {
  ASSERT_TRUE(Range("abc") != Range("bc"));
  ASSERT_FALSE(Range("bc") != Range("bc"));
}

TEST(RangeTest, InequalityOperatorTakesCString) {
  ASSERT_TRUE(Range("abc") != "bc");
  ASSERT_FALSE(Range("bc") != "bc");
}

TEST(RangeTest, InequalityOperatorTakesStdString) {
  ASSERT_TRUE(Range("abc") != std::string("bc"));
  ASSERT_FALSE(Range("bc") != std::string("bc"));
}

TEST(RangeTest, LessThanOperator) {
  ASSERT_TRUE(Range("abc") < Range("abcd"));
  ASSERT_FALSE(Range("bc") < Range("abcd"));
}

TEST(RangeTest, LessThanOperatorTakesCString) {
  ASSERT_TRUE(Range("abc") < "abcd");
  ASSERT_FALSE(Range("bc") < "abcd");
}

TEST(RangeTest, LessThanOperatorTakesStdString) {
  ASSERT_TRUE(Range("abc") < std::string("abcd"));
  ASSERT_FALSE(Range("bc") < std::string("abcd"));
}

}  // namespace multimap
