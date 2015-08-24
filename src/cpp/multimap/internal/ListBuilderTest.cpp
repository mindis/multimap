// This file is part of the InvertedIndex library.
//
// Copyright (C) 2015 Martin Trenkmann
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <type_traits>
#include "gmock/gmock.h"
#include "multimap/internal/ListBuilder.hpp"

namespace multimap {
namespace internal {

using testing::Eq;

TEST(ListBuilderTest, IsNotDefaultConstructible) {
  ASSERT_THAT(std::is_default_constructible<ListBuilder>::value, Eq(false));
}

}  // namespace internal
}  // namespace multimap
