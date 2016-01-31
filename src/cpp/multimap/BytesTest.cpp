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
#include "multimap/Bytes.hpp"

namespace multimap {

TEST(BytesTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<Bytes>::value);
}

TEST(BytesTest, IsCopyConstructibleOrAssignable) {
  ASSERT_TRUE(std::is_copy_constructible<Bytes>::value);
  ASSERT_TRUE(std::is_copy_assignable<Bytes>::value);
}

TEST(BytesTest, IsMoveConstructibleOrAssignable) {
  ASSERT_TRUE(std::is_move_constructible<Bytes>::value);
  ASSERT_TRUE(std::is_move_assignable<Bytes>::value);
}

TEST(BytesTest, EqualityOperator) {
  ASSERT_TRUE(Bytes("abc") == Bytes("abc"));
  ASSERT_FALSE(Bytes("bc") == Bytes("abc"));
}

TEST(BytesTest, EqualityOperatorTakesCString) {
  ASSERT_TRUE(Bytes("abc") == "abc");
  ASSERT_FALSE(Bytes("bc") == "abc");
}

TEST(BytesTest, EqualityOperatorTakesStdString) {
  ASSERT_TRUE(Bytes("abc") == std::string("abc"));
  ASSERT_FALSE(Bytes("bc") == std::string("abc"));
}

TEST(BytesTest, InequalityOperator) {
  ASSERT_TRUE(Bytes("abc") != Bytes("bc"));
  ASSERT_FALSE(Bytes("bc") != Bytes("bc"));
}

TEST(BytesTest, InequalityOperatorTakesCString) {
  ASSERT_TRUE(Bytes("abc") != "bc");
  ASSERT_FALSE(Bytes("bc") != "bc");
}

TEST(BytesTest, InequalityOperatorTakesStdString) {
  ASSERT_TRUE(Bytes("abc") != std::string("bc"));
  ASSERT_FALSE(Bytes("bc") != std::string("bc"));
}

TEST(BytesTest, LessThanOperator) {
  ASSERT_TRUE(Bytes("abc") < Bytes("abcd"));
  ASSERT_FALSE(Bytes("bc") < Bytes("abcd"));
}

TEST(BytesTest, LessThanOperatorTakesCString) {
  ASSERT_TRUE(Bytes("abc") < "abcd");
  ASSERT_FALSE(Bytes("bc") < "abcd");
}

TEST(BytesTest, LessThanOperatorTakesStdString) {
  ASSERT_TRUE(Bytes("abc") < std::string("abcd"));
  ASSERT_FALSE(Bytes("bc") < std::string("abcd"));
}

} // namespace multimap
