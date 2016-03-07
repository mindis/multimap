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
#include "multimap/internal/Block.hpp"
#include "multimap/internal/Generator.hpp"

namespace multimap {
namespace internal {

// -----------------------------------------------------------------------------
// class BasicBlock
// -----------------------------------------------------------------------------

TEST(ReadOnlyBlockTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<ReadOnlyBlock>::value);
}

TEST(ReadOnlyBlockTest, IsCopyConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_copy_constructible<ReadOnlyBlock>::value);
  ASSERT_TRUE(std::is_copy_assignable<ReadOnlyBlock>::value);
}

TEST(ReadOnlyBlockTest, DefaultConstructedHasProperState) {
  ASSERT_FALSE(ReadOnlyBlock().hasData());
  ASSERT_EQ(ReadOnlyBlock().data(), nullptr);
  ASSERT_EQ(ReadOnlyBlock().size(), 0);
  ASSERT_EQ(ReadOnlyBlock().offset(), 0);
  ASSERT_EQ(ReadOnlyBlock().remaining(), 0);
}

TEST(ReadOnlyBlockTest, ConstructedWithNullDataThrows) {
  ASSERT_THROW(ReadOnlyBlock(nullptr, 1), mt::AssertionError);
}

TEST(ReadOnlyBlockTest, ConstructedWithDataHasProperState) {
  byte buf[512];
  ASSERT_TRUE(ReadOnlyBlock(buf, sizeof buf).hasData());
  ASSERT_EQ(ReadOnlyBlock(buf, sizeof buf).data(), buf);
  ASSERT_EQ(ReadOnlyBlock(buf, sizeof buf).size(), sizeof buf);
  ASSERT_EQ(ReadOnlyBlock(buf, sizeof buf).offset(), 0);
  ASSERT_EQ(ReadOnlyBlock(buf, sizeof buf).remaining(), sizeof buf);
}

TEST(ReadWriteBlockTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<ReadWriteBlock>::value);
}

TEST(ReadWriteBlockTest, IsCopyConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_copy_constructible<ReadWriteBlock>::value);
  ASSERT_TRUE(std::is_copy_assignable<ReadWriteBlock>::value);
}

TEST(ReadWriteBlockTest, DefaultConstructedHasProperState) {
  ASSERT_FALSE(ReadWriteBlock().hasData());
  ASSERT_EQ(ReadWriteBlock().data(), nullptr);
  ASSERT_EQ(ReadWriteBlock().size(), 0);
  ASSERT_EQ(ReadWriteBlock().offset(), 0);
  ASSERT_EQ(ReadWriteBlock().remaining(), 0);
}

TEST(ReadWriteBlockTest, ConstructedWithNullDataThrows) {
  ASSERT_THROW(ReadWriteBlock(nullptr, 1), mt::AssertionError);
}

TEST(ReadWriteBlockTest, ConstructedWithDataHasProperState) {
  byte buf[512];
  ASSERT_TRUE(ReadWriteBlock(buf, sizeof buf).hasData());
  ASSERT_EQ(ReadWriteBlock(buf, sizeof buf).data(), buf);
  ASSERT_EQ(ReadWriteBlock(buf, sizeof buf).size(), sizeof buf);
  ASSERT_EQ(ReadWriteBlock(buf, sizeof buf).offset(), 0);
  ASSERT_EQ(ReadWriteBlock(buf, sizeof buf).remaining(), sizeof buf);
}

TEST(ReadWriteBlockTest, WriteToDefaultConstructedThrows) {
  ASSERT_THROW(ReadWriteBlock().writeData("x"), mt::AssertionError);
}

TEST(ReadWriteBlockTest, WriteValuesSmallerThanBlockSize) {
  byte buf[512];
  ReadWriteBlock block(buf, sizeof buf);

  std::string value = "123";
  while (true) {
    const auto remaining = block.remaining();
    const auto nbytes = block.writeData(value.c_str());
    if (remaining < value.size()) {
      ASSERT_EQ(nbytes, remaining);
      ASSERT_EQ(block.remaining(), 0);
      ASSERT_EQ(block.offset(), block.size());
      break;
    } else {
      ASSERT_EQ(nbytes, value.size());
    }
  }
}

TEST(ReadWriteBlockTest, WriteValuesLargerThanBlockSize) {
  byte buf[512];
  ReadWriteBlock block(buf, sizeof buf);

  std::string value(600, 'x');
  auto nbytes = block.writeData(value.c_str());
  ASSERT_EQ(nbytes, block.size());
  ASSERT_EQ(block.remaining(), 0);
  ASSERT_EQ(block.offset(), block.size());

  nbytes = block.writeData(value.c_str());
  ASSERT_EQ(nbytes, 0);
  ASSERT_EQ(block.remaining(), 0);
  ASSERT_EQ(block.offset(), block.size());

  block.rewind();
  nbytes = block.writeData(value.c_str());
  ASSERT_EQ(nbytes, block.size());
  ASSERT_EQ(block.remaining(), 0);
  ASSERT_EQ(block.offset(), block.size());
}

TEST(ReadWriteBlockTest, WriteValuesAndIterateOnce) {
  byte buf[512];
  ReadWriteBlock block(buf, sizeof buf);

  size_t num_values = 0;
  SequenceGenerator gen;
  while (true) {
    const bool flag = num_values % 2;
    const std::string value = gen.next();
    auto nbytes = block.writeSizeWithFlag(value.size(), flag);
    if (nbytes != 0) {
      nbytes = block.writeData(value.c_str());
      if (nbytes == value.size()) {
        ++num_values;
        continue;
      }
    }
    break;
  }
  ASSERT_GT(num_values, 0);

  gen.reset();
  block.rewind();

  bool flag;
  uint32_t size;
  for (size_t i = 0; i != num_values; ++i) {
    const std::string expected = gen.next();
    block.readSizeWithFlag(&size, &flag);
    ASSERT_EQ(size, expected.size());
    ASSERT_EQ(flag, i % 2);
    AutoBytes actual(size);
    block.readData(actual.data(), size);
    ASSERT_EQ(Bytes(actual), expected);
  }
}

TEST(ReadWriteBlockTest, WriteValuesAndFlipFlags) {
  byte buf[512];
  ReadWriteBlock block(buf, sizeof buf);

  size_t num_values = 0;
  SequenceGenerator gen;
  while (true) {
    const bool flag = num_values % 2;
    const std::string value = gen.next();
    const size_t offset = block.offset();
    if (block.writeSizeWithFlag(value.size(), flag)) {
      const size_t nbytes = block.writeData(value.c_str());
      if (nbytes == value.size()) {
        block.setFlagAt(!flag, offset);
        ++num_values;
        continue;
      }
    }
    break;
  }
  ASSERT_GT(num_values, 0);

  gen.reset();
  block.rewind();

  bool flag;
  uint32_t size;
  for (size_t i = 0; i != num_values; ++i) {
    const std::string expected = gen.next();
    block.readSizeWithFlag(&size, &flag);
    ASSERT_EQ(size, expected.size());
    ASSERT_EQ(flag, !(i % 2));
    AutoBytes actual(size);
    block.readData(actual.data(), size);
    ASSERT_EQ(Bytes(actual), expected);
  }
}

// -----------------------------------------------------------------------------
// class ExtendedBasicBlock
// -----------------------------------------------------------------------------

TEST(ExtendedReadOnlyBlockTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<ExtendedReadOnlyBlock>::value);
}

TEST(ExtendedReadOnlyBlockTest, IsCopyConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_copy_constructible<ExtendedReadOnlyBlock>::value);
  ASSERT_TRUE(std::is_copy_assignable<ExtendedReadOnlyBlock>::value);
}

TEST(ExtendedReadOnlyBlockTest, DefaultConstructedHasProperState) {
  ASSERT_EQ(ExtendedReadOnlyBlock().id, -1);
  ASSERT_FALSE(ExtendedReadOnlyBlock().ignore);
}

TEST(ExtendedReadWriteBlockTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<ExtendedReadWriteBlock>::value);
}

TEST(ExtendedReadWriteBlockTest, IsCopyConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_copy_constructible<ExtendedReadWriteBlock>::value);
  ASSERT_TRUE(std::is_copy_assignable<ExtendedReadWriteBlock>::value);
}

TEST(ExtendedReadWriteBlockTest, DefaultConstructedHasProperState) {
  ASSERT_EQ(ExtendedReadWriteBlock().id, -1);
  ASSERT_FALSE(ExtendedReadWriteBlock().ignore);
}

}  // namespace internal
}  // namespace multimap
