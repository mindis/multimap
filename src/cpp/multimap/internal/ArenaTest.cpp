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
#include "multimap/internal/Arena.hpp"
#include "multimap/thirdparty/mt/mt.hpp"

namespace multimap {
namespace internal {

TEST(ArenaTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<Arena>::value);
}

TEST(ArenaTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<Arena>::value);
  ASSERT_FALSE(std::is_copy_assignable<Arena>::value);
}

TEST(ArenaTest, IsMoveConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_move_constructible<Arena>::value);
  ASSERT_TRUE(std::is_move_assignable<Arena>::value);
}

TEST(ArenaTest, DefaultConstructedHasProperState) {
  ASSERT_THROW(Arena().allocate(0), mt::AssertionError);
  ASSERT_NE(Arena().allocate(1), nullptr);
  ASSERT_EQ(Arena().allocated(), 0);
}

TEST(ArenaTest, ConstructedWithValidParamsHasProperState) {
  Arena arena;
  ASSERT_THROW(arena.allocate(0), mt::AssertionError);
  ASSERT_NE(arena.allocate(1), nullptr);
  ASSERT_EQ(arena.allocated(), 1);
  ASSERT_NE(arena.allocate(2), nullptr);
  ASSERT_EQ(arena.allocated(), 3);
  ASSERT_NE(arena.allocate(128), nullptr);
  ASSERT_EQ(arena.allocated(), 131);
  ASSERT_NE(arena.allocate(5000), nullptr);
  ASSERT_EQ(arena.allocated(), 5131);
}

} // namespace internal
} // namespace multimap
