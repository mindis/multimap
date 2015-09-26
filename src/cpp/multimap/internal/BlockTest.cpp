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

#include <unistd.h>
#include <array>
#include <algorithm>
#include <cstring>
#include <type_traits>
#include <boost/filesystem/operations.hpp>
#include "gmock/gmock.h"
#include "multimap/internal/Block.hpp"
#include "multimap/internal/Generator.hpp"
#include "multimap/internal/System.hpp"

namespace multimap {
namespace internal {

using testing::Eq;
using testing::Ne;
using testing::Gt;
using testing::Le;
using testing::IsNull;
using testing::NotNull;

typedef Block::Iterator ListIter;
typedef Block::ConstIterator ListConstIter;

class BlockTestParam : public testing::TestWithParam<std::uint32_t> {};

TEST(BlockIterTest, IsDefaultConstructible) {
  ASSERT_THAT(std::is_default_constructible<ListIter>::value, Eq(true));
}

TEST(BlockIterTest, IsCopyConstructibleAndAssignable) {
  ASSERT_THAT(std::is_copy_constructible<ListIter>::value, Eq(true));
  ASSERT_THAT(std::is_copy_assignable<ListIter>::value, Eq(true));
}

TEST(BlockIterTest, IsMoveConstructibleAndAssignable) {
  ASSERT_THAT(std::is_move_constructible<ListIter>::value, Eq(true));
  ASSERT_THAT(std::is_move_assignable<ListIter>::value, Eq(true));
}

TEST(BlockIterTest, DefaultConstructedHasProperState) {
  ASSERT_THAT(ListIter().hasNext(), Eq(false));
}

TEST(BlockConstIterTest, IsDefaultConstructible) {
  ASSERT_THAT(std::is_default_constructible<ListConstIter>::value, Eq(true));
}

TEST(BlockConstIterTest, IsCopyConstructibleAndAssignable) {
  ASSERT_THAT(std::is_copy_constructible<ListConstIter>::value, Eq(true));
  ASSERT_THAT(std::is_copy_assignable<ListConstIter>::value, Eq(true));
}

TEST(BlockConstIterTest, IsMoveConstructibleAndAssignable) {
  ASSERT_THAT(std::is_move_constructible<ListConstIter>::value, Eq(true));
  ASSERT_THAT(std::is_move_assignable<ListConstIter>::value, Eq(true));
}

TEST(BlockConstIterTest, DefaultConstructedHasProperState) {
  ASSERT_THAT(ListConstIter().hasNext(), Eq(false));
}

TEST(BlockTest, IsDefaultConstructible) {
  ASSERT_THAT(std::is_default_constructible<Block>::value, Eq(true));
}

TEST(BlockTest, IsCopyConstructibleAndAssignable) {
  ASSERT_THAT(std::is_copy_constructible<Block>::value, Eq(true));
  ASSERT_THAT(std::is_copy_assignable<Block>::value, Eq(true));
}

TEST(BlockTest, IsMoveConstructibleAndAssignable) {
  ASSERT_THAT(std::is_move_constructible<Block>::value, Eq(true));
  ASSERT_THAT(std::is_move_assignable<Block>::value, Eq(true));
}

TEST(BlockTest, DefaultConstructedHasProperState) {
  ASSERT_THAT(Block().data(), IsNull());
  ASSERT_THAT(Block().size(), Eq(0));
  ASSERT_THAT(Block().position(), Eq(0));
  ASSERT_THAT(Block().hasData(), Eq(false));
  ASSERT_THAT(Block().load_factor(), Eq(0));
  ASSERT_THAT(Block().max_value_size(), Eq(0));
}

TEST(BlockTest, ConstructedWithNullDataDies) {
  ASSERT_DEATH(Block(nullptr, 1), "");
}

TEST(BlockTest, ConstructedWithNullSizeDies) {
  char data[0];
  ASSERT_DEATH(Block(data, sizeof data), "");
}

TEST(BlockTest, AddToDefaultConstructedDies) {
  Block block;
  ASSERT_DEATH(block.add(Bytes()), "");
}

TEST(BlockTest, AddMaxValueToEmptyBlockWorks) {
  char buf[512];
  Block block(buf, sizeof buf);
  std::string value(block.max_value_size(), '\0');
  ASSERT_TRUE(block.add(value));
}

TEST(BlockTest, AddTooLargeValueToEmptyBlockThrows) {
  char buf[512];
  Block block(buf, sizeof buf);
  std::string value(block.max_value_size() + 1, '\0');
  ASSERT_THROW(block.add(value), std::runtime_error);
}

// TODO Remove duplication
TEST(BlockTest, AddingValuesIncreasesLoadFactor) {
  const auto num_values = 100;
  const auto value_size = 23;
  const auto block_size =
      (Block::SIZE_OF_VALUE_SIZE_FIELD + value_size) * num_values;
  char block_data[block_size];
  Block block(block_data, block_size);
  auto generator = SequenceGenerator::create();
  double prev_load_factor = 0;
  for (auto i = 0; i != num_values; ++i) {
    ASSERT_TRUE(block.add(generator->generate(value_size)));
    ASSERT_THAT(block.load_factor(), Gt(prev_load_factor));
    prev_load_factor = block.load_factor();
  }
}

// TODO Remove duplication
TEST(BlockTest, AddValuesAndIterateAll) {
  const auto num_values = 100;
  const auto value_size = 23;
  const auto block_size =
      (Block::SIZE_OF_VALUE_SIZE_FIELD + value_size) * num_values;
  char block_data[block_size];
  Block block(block_data, block_size);
  auto generator = SequenceGenerator::create();
  for (auto i = 0; i != num_values; ++i) {
    ASSERT_TRUE(block.add(generator->generate(value_size)));
  }
  ASSERT_FALSE(block.add(generator->generate(value_size)));

  generator->reset();
  auto iter = block.iterator();
  for (auto i = 0; i != num_values; ++i) {
    ASSERT_TRUE(iter.hasNext());
    ASSERT_THAT(iter.next(), Eq(generator->generate(value_size)));
  }
  ASSERT_FALSE(iter.hasNext());
}

TEST_P(BlockTestParam, AddingValuesIncreasesLoadFactor) {
  const auto num_values = 100;
  const auto value_size = GetParam();
  const auto block_size =
      (Block::SIZE_OF_VALUE_SIZE_FIELD + value_size) * num_values;
  char block_data[block_size];
  Block block(block_data, block_size);
  auto generator = SequenceGenerator::create();
  double prev_load_factor = 0;
  for (auto i = 0; i != num_values; ++i) {
    ASSERT_TRUE(block.add(generator->generate(value_size)));
    ASSERT_THAT(block.load_factor(), Gt(prev_load_factor));
    prev_load_factor = block.load_factor();
  }
}

TEST_P(BlockTestParam, AddValuesAndIterateAll) {
  const auto num_values = 100;
  const auto value_size = GetParam();
  const auto block_size =
      (Block::SIZE_OF_VALUE_SIZE_FIELD + value_size) * num_values;
  char block_data[block_size];
  Block block(block_data, block_size);
  auto generator = SequenceGenerator::create();
  for (auto i = 0; i != num_values; ++i) {
    ASSERT_TRUE(block.add(generator->generate(value_size)));
  }
  ASSERT_FALSE(block.add(generator->generate(value_size)));

  generator->reset();
  auto iter = block.iterator();
  for (auto i = 0; i != num_values; ++i) {
    ASSERT_TRUE(iter.hasNext());
    ASSERT_THAT(iter.next(), Eq(generator->generate(value_size)));
  }
  ASSERT_FALSE(iter.hasNext());
}

TEST_P(BlockTestParam, AddValuesAndDeleteEvery2ndWhileIterating) {
  const auto num_values = 100;
  const auto value_size = GetParam();
  const auto block_size =
      (Block::SIZE_OF_VALUE_SIZE_FIELD + value_size) * num_values;
  char block_data[block_size];
  Block block(block_data, block_size);
  auto generator = SequenceGenerator::create();
  for (auto i = 0; i != num_values; ++i) {
    ASSERT_TRUE(block.add(generator->generate(value_size)));
  }

  auto iter = block.iterator();
  for (auto i = 0; i != num_values; ++i) {
    ASSERT_TRUE(iter.hasNext());
    iter.next();
    if (i % 2 == 0) {
      iter.markAsDeleted();
    }
  }
  ASSERT_FALSE(iter.hasNext());

  generator->reset();
  iter = block.iterator();
  for (auto i = 0; i != num_values; ++i) {
    const auto expected_value = generator->generate(value_size);
    if (i % 2 != 0) {
      ASSERT_TRUE(iter.hasNext());
      ASSERT_THAT(iter.next(), Eq(expected_value));
    }
  }
  ASSERT_FALSE(iter.hasNext());
}

INSTANTIATE_TEST_CASE_P(Parameterized, BlockTestParam,
                        testing::Values(100, 10000));

}  // namespace internal
}  // namespace multimap
