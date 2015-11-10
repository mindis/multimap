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
#include "multimap/internal/ListLock.hpp"

namespace multimap {
namespace internal {

TEST(ListLockTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<SharedListLock>::value);
  ASSERT_TRUE(std::is_default_constructible<UniqueListLock>::value);
}

TEST(ListLockTest, DefaultConstructedHasProperState) {
  ASSERT_EQ(SharedListLock().list(), nullptr);
  ASSERT_EQ(UniqueListLock().list(), nullptr);
}

TEST(ListLockTest, IsMoveConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_move_constructible<SharedListLock>::value);
  ASSERT_TRUE(std::is_move_assignable<SharedListLock>::value);

  ASSERT_TRUE(std::is_move_constructible<UniqueListLock>::value);
  ASSERT_TRUE(std::is_move_assignable<UniqueListLock>::value);
}

TEST(ListLockTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<SharedListLock>::value);
  ASSERT_FALSE(std::is_copy_assignable<SharedListLock>::value);

  ASSERT_FALSE(std::is_copy_constructible<UniqueListLock>::value);
  ASSERT_FALSE(std::is_copy_assignable<UniqueListLock>::value);
}

}  // namespace internal
}  // namespace multimap
