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

#include <string>
#include <type_traits>
#include "gmock/gmock.h"
#include "multimap/Slice.h"

namespace multimap {

TEST(SliceTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<Slice>::value);
}

TEST(SliceTest, IsCopyConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_copy_constructible<Slice>::value);
  ASSERT_TRUE(std::is_copy_assignable<Slice>::value);
}

TEST(SliceTest, IsMoveConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_move_constructible<Slice>::value);
  ASSERT_TRUE(std::is_move_assignable<Slice>::value);
}

TEST(SliceTest, EqualityOperator) {
  ASSERT_EQ(Slice("abc"), Slice("abc"));
  ASSERT_NE(Slice("bc"), Slice("abc"));
}

TEST(SliceTest, EqualityOperatorTakesCString) {
  ASSERT_EQ(Slice("abc"), "abc");
  ASSERT_NE(Slice("bc"), "abc");
}

TEST(SliceTest, EqualityOperatorTakesStdString) {
  ASSERT_EQ(Slice("abc"), std::string("abc"));
  ASSERT_NE(Slice("bc"), std::string("abc"));
}

TEST(SliceTest, InequalityOperator) {
  ASSERT_NE(Slice("abc"), Slice("bc"));
  ASSERT_EQ(Slice("bc"), Slice("bc"));
}

TEST(SliceTest, InequalityOperatorTakesCString) {
  ASSERT_NE(Slice("abc"), "bc");
  ASSERT_EQ(Slice("bc"), "bc");
}

TEST(SliceTest, InequalityOperatorTakesStdString) {
  ASSERT_NE(Slice("abc"), std::string("bc"));
  ASSERT_EQ(Slice("bc"), std::string("bc"));
}

TEST(SliceTest, LessThanOperator) {
  ASSERT_LT(Slice("abc"), Slice("abcd"));
  ASSERT_GE(Slice("bc"), Slice("abcd"));
}

TEST(SliceTest, LessThanOperatorTakesCString) {
  ASSERT_LT(Slice("abc"), "abcd");
  ASSERT_GE(Slice("bc"), "abcd");
}

TEST(SliceTest, LessThanOperatorTakesStdString) {
  ASSERT_LT(Slice("abc"), std::string("abcd"));
  ASSERT_GE(Slice("bc"), std::string("abcd"));
}

}  // namespace multimap
