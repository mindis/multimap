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
using testing::Gt;
using testing::IsNull;
using testing::NotNull;

byte data[] = "data";
Block non_empty_block(data, sizeof data);

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
  BlockPool block_pool(23, 42);
  ASSERT_THAT(block_pool.num_blocks(), Eq(23));
  ASSERT_THAT(block_pool.block_size(), Eq(42));
  ASSERT_THAT(block_pool.num_blocks_free(), Eq(23));
  ASSERT_THAT(block_pool.memory(), Eq(23 * 42));
  ASSERT_THAT(block_pool.empty(), Eq(false));
  ASSERT_THAT(block_pool.full(), Eq(true));
  ASSERT_THAT(block_pool.Pop().data(), NotNull());
  ASSERT_DEATH(block_pool.Push(Block()), "");
  ASSERT_DEATH(block_pool.Push(std::move(non_empty_block)), "");
}

TEST(BlockPoolTest, PopUntilEmptyMeetsInvariants) {
  const auto num_blocks = 23;
  const auto block_size = 42;
  std::vector<Block> popped;
  BlockPool block_pool(num_blocks, block_size);
  while (!block_pool.empty()) {
    popped.push_back(block_pool.Pop());
    ASSERT_THAT(block_pool.num_blocks(), Eq(num_blocks));
    ASSERT_THAT(block_pool.block_size(), Eq(block_size));
    ASSERT_THAT(block_pool.num_blocks_free(), Eq(num_blocks - popped.size()));
    ASSERT_THAT(block_pool.memory(), Eq(num_blocks * block_size));
    ASSERT_THAT(block_pool.full(), Eq(false));
  }
  ASSERT_THAT(popped.size(), Eq(num_blocks));
  ASSERT_THAT(block_pool.Pop().data(), IsNull());
}

TEST(BlockPoolTest, PushUntilFullMeetsInvariants) {
  const auto num_blocks = 23;
  const auto block_size = 42;
  std::vector<Block> popped;
  BlockPool block_pool(num_blocks, block_size);
  while (!block_pool.empty()) {
    popped.push_back(block_pool.Pop());
  }
  ASSERT_THAT(popped.size(), Eq(num_blocks));

  while (!block_pool.full()) {
    ASSERT_NO_THROW(block_pool.Push(std::move(popped.back())));
    popped.pop_back();
    ASSERT_THAT(block_pool.num_blocks(), Eq(num_blocks));
    ASSERT_THAT(block_pool.block_size(), Eq(block_size));
    ASSERT_THAT(block_pool.num_blocks_free(), Eq(num_blocks - popped.size()));
    ASSERT_THAT(block_pool.memory(), Eq(num_blocks * block_size));
    ASSERT_THAT(block_pool.empty(), Eq(false));

    ASSERT_DEATH(block_pool.Push(Block()), "");
    ASSERT_DEATH(block_pool.Push(std::move(non_empty_block)), "");
  }
  ASSERT_THAT(block_pool.full(), Eq(true));
}

}  // namespace internal
}  // namespace multimap
