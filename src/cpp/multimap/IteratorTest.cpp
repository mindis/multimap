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
#include "multimap/Iterator.hpp"

namespace multimap {

using testing::Eq;

TEST(IteratorTest, IsNotDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<Iterator>::value);
}

TEST(IteratorTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<Iterator>::value);
  ASSERT_FALSE(std::is_copy_assignable<Iterator>::value);
}

TEST(IteratorTest, IsNotMoveConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_move_constructible<Iterator>::value);
  ASSERT_FALSE(std::is_move_assignable<Iterator>::value);
}

TEST(IteratorTest, EmptyInstanceHasNoValues) {
  ASSERT_THAT(Iterator::newEmptyInstance()->available(), Eq(0));
  ASSERT_FALSE(Iterator::newEmptyInstance()->hasNext());
}

}  // namespace multimap
