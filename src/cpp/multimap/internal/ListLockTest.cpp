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
#include "multimap/internal/ListLock.hpp"

namespace multimap {
namespace internal {

using testing::Eq;
using testing::IsNull;

TEST(ListLockTest, IsDefaultConstructible) {
  ASSERT_THAT(std::is_default_constructible<SharedListLock>::value, Eq(true));
  ASSERT_THAT(std::is_default_constructible<UniqueListLock>::value, Eq(true));
}

TEST(ListLockTest, DefaultConstructedHasProperState) {
  ASSERT_THAT(SharedListLock().clist(), IsNull());
  ASSERT_THAT(UniqueListLock().clist(), IsNull());
}

TEST(ListLockTest, ConstructionWithNullArgumentsFails) {
  ASSERT_DEATH(UniqueListLock(nullptr), "");
}

TEST(ListLockTest, IsMoveConstructibleAndAssignable) {
  ASSERT_THAT(std::is_move_constructible<SharedListLock>::value, Eq(true));
  ASSERT_THAT(std::is_move_assignable<SharedListLock>::value, Eq(true));

  ASSERT_THAT(std::is_move_constructible<UniqueListLock>::value, Eq(true));
  ASSERT_THAT(std::is_move_assignable<UniqueListLock>::value, Eq(true));
}

TEST(ListLockTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_THAT(std::is_copy_constructible<SharedListLock>::value, Eq(false));
  ASSERT_THAT(std::is_copy_assignable<SharedListLock>::value, Eq(false));

  ASSERT_THAT(std::is_copy_constructible<UniqueListLock>::value, Eq(false));
  ASSERT_THAT(std::is_copy_assignable<UniqueListLock>::value, Eq(false));
}

}  // namespace internal
}  // namespace multimap
