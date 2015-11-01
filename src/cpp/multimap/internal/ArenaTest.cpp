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
#include "multimap/internal/Arena.hpp"
#include "multimap/thirdparty/mt.hpp"

namespace multimap {
namespace internal {

using testing::Eq;
using testing::NotNull;

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

TEST(ArenaTest, ConstructedWithValidParamsHasProperState) {
  const auto chunk_size = 128;
  Arena arena(chunk_size);
  ASSERT_THROW(arena.allocate(0), mt::AssertionError);
  ASSERT_THAT(arena.allocate(1), NotNull());
  ASSERT_THAT(arena.allocated(), Eq(1));
  ASSERT_THAT(arena.allocate(2), NotNull());
  ASSERT_THAT(arena.allocated(), Eq(3));
  ASSERT_THAT(arena.allocate(128), NotNull());
  ASSERT_THAT(arena.allocated(), Eq(131));
  ASSERT_THAT(arena.allocate(5000), NotNull());
  ASSERT_THAT(arena.allocated(), Eq(5131));
}

}  // namespace internal
}  // namespace multimap
