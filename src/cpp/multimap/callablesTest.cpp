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
#include "multimap/callables.hpp"

namespace multimap {

TEST(CallablesTest, TestEqual) {
  ASSERT_TRUE(Equal("")(""));
  ASSERT_TRUE(Equal("abc")("abc"));

  ASSERT_FALSE(Equal("")("abc"));
  ASSERT_FALSE(Equal("abc")(""));
  ASSERT_FALSE(Equal("ab")("c"));
}

TEST(CallablesTest, TestContains) {
  ASSERT_TRUE(Contains("")(""));
  ASSERT_TRUE(Contains("")("abc"));
  ASSERT_TRUE(Contains("abc")("abc"));
  ASSERT_TRUE(Contains("abc")("abcd"));
  ASSERT_TRUE(Contains("abc")("zabc"));

  ASSERT_FALSE(Contains("abc")(""));
  ASSERT_FALSE(Contains("abc")("ab"));
}

TEST(CallablesTest, TestStartsWith) {
  ASSERT_TRUE(StartsWith("")(""));
  ASSERT_TRUE(StartsWith("abc")("abc"));
  ASSERT_TRUE(StartsWith("abc")("abcd"));
  ASSERT_FALSE(StartsWith("bc")("abc"));
}

TEST(CallablesTest, TestEndsWith) {
  ASSERT_TRUE(EndsWith("")(""));
  ASSERT_TRUE(EndsWith("abc")("abc"));
  ASSERT_TRUE(EndsWith("abc")("zabc"));
  ASSERT_FALSE(EndsWith("bc")("abcd"));
}

}  // namespace multimap
