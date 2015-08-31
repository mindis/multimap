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

#include <limits>
#include <type_traits>
#include <vector>
#include "gmock/gmock.h"
#include "multimap/internal/BlockPool.hpp"

namespace multimap {
namespace internal {

using testing::Eq;
using testing::NotNull;

TEST(BlockPoolTest, IsDefaultConstructible) {
  ASSERT_THAT(std::is_default_constructible<BlockPool>::value, Eq(true));
}

TEST(BlockPoolTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_THAT(std::is_copy_constructible<BlockPool>::value, Eq(false));
  ASSERT_THAT(std::is_copy_assignable<BlockPool>::value, Eq(false));
}

TEST(BlockPoolTest, IsNotMoveConstructibleOrAssignable) {
  ASSERT_THAT(std::is_move_constructible<BlockPool>::value, Eq(false));
  ASSERT_THAT(std::is_move_assignable<BlockPool>::value, Eq(false));
}

TEST(BlockPoolTest, ConstructedWithValidParamsHasProperState) {
  const auto block_size = 128;
  const auto chunk_size = MiB(10);
  BlockPool block_pool(block_size, chunk_size);
  ASSERT_THAT(block_pool.block_size(), Eq(block_size));
  ASSERT_THAT(block_pool.num_blocks(), Eq(81920));
  ASSERT_THAT(block_pool.Allocate().data(), NotNull());
}

}  // namespace internal
}  // namespace multimap
