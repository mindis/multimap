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

//class ReadOnlyBlockParam : public testing::TestWithParam<std::uint32_t> {};

TEST(ReadOnlyBlockTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<ReadOnlyBlock>::value);
}

TEST(ReadOnlyBlockTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<ReadOnlyBlock>::value);
  ASSERT_FALSE(std::is_copy_assignable<ReadOnlyBlock>::value);
}

TEST(ReadOnlyBlockTest, IsMoveConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_move_constructible<ReadOnlyBlock>::value);
  ASSERT_TRUE(std::is_move_assignable<ReadOnlyBlock>::value);
}

TEST(ReadOnlyBlockTest, DefaultConstructedHasProperState) {
  ASSERT_THAT(ReadOnlyBlock().hasData(), Eq(false));
  ASSERT_THAT(ReadOnlyBlock().data(), IsNull());
  ASSERT_THAT(ReadOnlyBlock().size(), Eq(0));
  ASSERT_THAT(ReadOnlyBlock().offset(), Eq(0));
  ASSERT_THAT(ReadOnlyBlock().remaining(), Eq(0));
}

TEST(ReadOnlyBlockTest, ConstructedWithNullDataThrows) {
  ASSERT_THROW(ReadOnlyBlock(nullptr, 1), mt::AssertionError);
}

TEST(ReadWriteBlockTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<ReadWriteBlock>::value);
}

TEST(ReadWriteBlockTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<ReadWriteBlock>::value);
  ASSERT_FALSE(std::is_copy_assignable<ReadWriteBlock>::value);
}

TEST(ReadWriteBlockTest, IsMoveConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_move_constructible<ReadWriteBlock>::value);
  ASSERT_TRUE(std::is_move_assignable<ReadWriteBlock>::value);
}

TEST(ReadWriteBlockTest, DefaultConstructedHasProperState) {
  ASSERT_THAT(ReadWriteBlock().hasData(), Eq(false));
  ASSERT_THAT(ReadWriteBlock().data(), IsNull());
  ASSERT_THAT(ReadWriteBlock().size(), Eq(0));
  ASSERT_THAT(ReadWriteBlock().offset(), Eq(0));
  ASSERT_THAT(ReadWriteBlock().remaining(), Eq(0));
}

TEST(ReadWriteBlockTest, ConstructedWithNullDataThrows) {
  ASSERT_THROW(ReadWriteBlock(nullptr, 1), mt::AssertionError);
}





//TEST(BlockTest, AddToDefaultConstructedDies) {
//  Block block;
//  ASSERT_THROW(block.add(Bytes()), mt::AssertionError);
//}

//TEST(BlockTest, AddTooLargeValueToEmptyBlockThrows) {
//  char buf[512];
//  Block block(buf, sizeof buf);
//  std::string value(block.max_value_size() + 1, '\0');
//  ASSERT_THROW(block.add(value), std::runtime_error);
//}

// TODO Remove duplication
//TEST(BlockTest, AddValuesAndIterateAll) {
//  const auto num_values = 100;
//  const auto value_size = 23;
//  const auto block_size =
//      (Block::SIZE_OF_VALUE_SIZE_FIELD + value_size) * num_values;
//  char block_data[block_size];
//  Block block(block_data, block_size);
//  auto generator = SequenceGenerator::create();
//  for (auto i = 0; i != num_values; ++i) {
//    ASSERT_TRUE(block.add(generator->generate(value_size)));
//  }
//  ASSERT_FALSE(block.add(generator->generate(value_size)));

//  generator->reset();
//  auto iter = block.iterator();
//  for (auto i = 0; i != num_values; ++i) {
//    ASSERT_TRUE(iter.hasNext());
//    ASSERT_THAT(iter.next(), Eq(generator->generate(value_size)));
//  }
//  ASSERT_FALSE(iter.hasNext());
//}

//TEST_P(BlockTestParam, AddValuesAndIterateAll) {
//  const auto num_values = 100;
//  const auto value_size = GetParam();
//  const auto block_size =
//      (Block::SIZE_OF_VALUE_SIZE_FIELD + value_size) * num_values;
//  char block_data[block_size];
//  Block block(block_data, block_size);
//  auto generator = SequenceGenerator::create();
//  for (auto i = 0; i != num_values; ++i) {
//    ASSERT_TRUE(block.add(generator->generate(value_size)));
//  }
//  ASSERT_FALSE(block.add(generator->generate(value_size)));

//  generator->reset();
//  auto iter = block.iterator();
//  for (auto i = 0; i != num_values; ++i) {
//    ASSERT_TRUE(iter.hasNext());
//    ASSERT_THAT(iter.next(), Eq(generator->generate(value_size)));
//  }
//  ASSERT_FALSE(iter.hasNext());
//}

//TEST_P(BlockTestParam, AddValuesAndDeleteEvery2ndWhileIterating) {
//  const auto num_values = 100;
//  const auto value_size = GetParam();
//  const auto block_size =
//      (Block::SIZE_OF_VALUE_SIZE_FIELD + value_size) * num_values;
//  char block_data[block_size];
//  Block block(block_data, block_size);
//  auto generator = SequenceGenerator::create();
//  for (auto i = 0; i != num_values; ++i) {
//    ASSERT_TRUE(block.add(generator->generate(value_size)));
//  }

//  auto iter = block.iterator();
//  for (auto i = 0; i != num_values; ++i) {
//    ASSERT_TRUE(iter.hasNext());
//    iter.next();
//    if (i % 2 == 0) {
//      iter.markAsDeleted();
//    }
//  }
//  ASSERT_FALSE(iter.hasNext());

//  generator->reset();
//  iter = block.iterator();
//  for (auto i = 0; i != num_values; ++i) {
//    const auto expected_value = generator->generate(value_size);
//    if (i % 2 != 0) {
//      ASSERT_TRUE(iter.hasNext());
//      ASSERT_THAT(iter.next(), Eq(expected_value));
//    }
//  }
//  ASSERT_FALSE(iter.hasNext());
//}

//INSTANTIATE_TEST_CASE_P(Parameterized, ReadOnlyBlockParam,
//                        testing::Values(100, 10000));

}  // namespace internal
}  // namespace multimap
