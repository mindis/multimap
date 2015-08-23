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
#include "multimap/internal/System.hpp"

namespace multimap {
namespace internal {

namespace {

template <std::size_t Size>
struct ByteArray : public std::array<byte, Size> {
  static const std::size_t kSize = Size;

  ByteArray() {
    for (std::size_t i = 0; i != Size; ++i) {
      (*this)[i] = std::numeric_limits<byte>::max() - i;
    }
  }

  Bytes ToBytes() const { return Bytes(this->data(), this->size()); }
};

typedef ByteArray<170> ByteArray170;      // ::kSize -> 10101010
typedef ByteArray<21845> ByteArray21845;  // ::kSize -> 101010101010101

static_assert(
    (ByteArray170::kSize >
     static_cast<std::size_t>(std::numeric_limits<std::int8_t>::max())) &&
        (ByteArray170::kSize <=
         static_cast<std::size_t>(std::numeric_limits<std::uint8_t>::max())),
    "size of ByteArray100 does not need 8 bits to be stored");

static_assert(
    (ByteArray21845::kSize >
     static_cast<std::size_t>(std::numeric_limits<std::uint8_t>::max())) &&
        (ByteArray21845::kSize <=
         static_cast<std::size_t>(std::numeric_limits<std::int16_t>::max())),
    "size of ByteArray1000 does not need 15 bits to be stored");

}  // namespace

using testing::Eq;
using testing::Ne;
using testing::Gt;
using testing::Le;
using testing::IsNull;
using testing::NotNull;

typedef Block::Iterator ListIter;
typedef Block::ConstIterator ListConstIter;

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
  ASSERT_THAT(ListIter().has_value(), Eq(false));
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
  ASSERT_THAT(ListConstIter().has_value(), Eq(false));
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
  ASSERT_THAT(Block().used(), Eq(0));
  ASSERT_THAT(Block().has_data(), Eq(false));
  ASSERT_THAT(Block().load_factor(), Eq(0));
  ASSERT_THAT(Block().max_value_size(), Eq(0));
}

TEST(BlockTest, ConstructedWithNullDataDies) {
  ASSERT_DEATH(Block(nullptr, 1), "");
}

TEST(BlockTest, ConstructedWithNullSizeDies) {
  byte data[1];
  ASSERT_DEATH(Block(data, 0), "");
}

TEST(BlockTest, AddToDefaultConstructedDies) {
  Block block;
  ASSERT_DEATH(block.Add(Bytes()), "");
}

TEST(BlockTest, AddMaxValueToEmptyBlockWorks) {
  byte buf[512];
  Block block(buf, sizeof buf);
  std::string value(block.max_value_size(), '\0');
  ASSERT_TRUE(block.Add(value));
}

TEST(BlockTest, AddTooLargeValueToEmptyBlockThrows) {
  byte buf[512];
  Block block(buf, sizeof buf);
  std::string value(block.max_value_size() + 1, '\0');
  ASSERT_THROW(block.Add(value), std::runtime_error);
}

TEST(BlockTest, AddSmallValuesIncreasesLoadFactor) {
  const auto num_values = 100;
  const auto required_size =
      (Block::kSizeOfValueSizeField + ByteArray170::kSize) * num_values;
  byte data[required_size];
  Block block(data, sizeof data);
  double prev_load_factor = 0;
  for (auto i = 0; i != num_values; ++i) {
    ByteArray170 value;
    ASSERT_THAT(block.Add(value.ToBytes()), Eq(true));
    ASSERT_THAT(block.load_factor(), Gt(prev_load_factor));
    prev_load_factor = block.load_factor();
  }
}

TEST(BlockTest, AddLargeValuesIncreasesLoadFactor) {
  const auto num_values = 100;
  const auto required_size =
      (Block::kSizeOfValueSizeField + ByteArray21845::kSize) * num_values;
  byte data[required_size];
  Block block(data, sizeof data);
  double prev_load_factor = 0;
  for (auto i = 0; i != num_values; ++i) {
    ByteArray21845 value;
    ASSERT_THAT(block.Add(value.ToBytes()), Eq(true));
    ASSERT_THAT(block.load_factor(), Gt(prev_load_factor));
    prev_load_factor = block.load_factor();
  }
}

TEST(BlockTest, AddSmallValuesAndIterateAll) {
  const auto num_values = 100;
  const auto required_size =
      (Block::kSizeOfValueSizeField + ByteArray170::kSize) * num_values;
  byte data[required_size];
  Block block(data, sizeof data);
  for (auto i = 0; i != num_values; ++i) {
    ByteArray170 value;
    ASSERT_THAT(block.Add(value.ToBytes()), Eq(true));
  }
  ByteArray170 value;
  ASSERT_THAT(block.Add(value.ToBytes()), Eq(false));

  auto iter = block.NewIterator();
  for (auto i = 0; i != num_values; ++i) {
    ByteArray170 value;
    ASSERT_THAT(iter.has_value(), Eq(true));

    ASSERT_THAT(iter.value().size(), Eq(value.ToBytes().size()));

    ASSERT_THAT(iter.value(), Eq(value.ToBytes()));
    ASSERT_THAT(iter.deleted(), Eq(false));
    iter.advance();
  }
  ASSERT_THAT(iter.has_value(), Eq(false));
}

TEST(BlockTest, AddLargeValuesAndIterateAll) {
  const auto num_values = 100;
  const auto required_size =
      (Block::kSizeOfValueSizeField + ByteArray21845::kSize) * num_values;
  byte data[required_size];
  Block block(data, sizeof data);
  for (auto i = 0; i != num_values; ++i) {
    ByteArray21845 value;
    ASSERT_THAT(block.Add(value.ToBytes()), Eq(true));
  }
  ByteArray21845 value;
  ASSERT_THAT(block.Add(value.ToBytes()), Eq(false));

  auto iter = block.NewIterator();
  for (auto i = 0; i != num_values; ++i) {
    ByteArray21845 value;
    ASSERT_THAT(iter.has_value(), Eq(true));
    ASSERT_THAT(iter.value(), Eq(value.ToBytes()));
    ASSERT_THAT(iter.deleted(), Eq(false));
    iter.advance();
  }
  ASSERT_THAT(iter.has_value(), Eq(false));
}

TEST(BlockTest, AddSmallValuesAndDeleteEvery2ndWhileIterating) {
  const auto num_values = 100;
  const auto required_size =
      (Block::kSizeOfValueSizeField + ByteArray170::kSize) * num_values;
  byte data[required_size];
  Block block(data, sizeof data);
  for (auto i = 0; i != num_values; ++i) {
    ByteArray170 value;
    ASSERT_THAT(block.Add(value.ToBytes()), Eq(true));
  }

  auto iter = block.NewIterator();
  for (auto i = 0; i != num_values; ++i) {
    ASSERT_THAT(iter.has_value(), Eq(true));
    if (i % 2 == 0) {
      iter.set_deleted();
    }
    iter.advance();
  }
  ASSERT_THAT(iter.has_value(), Eq(false));

  iter = block.NewIterator();
  for (auto i = 0; i != num_values; ++i) {
    ASSERT_THAT(iter.has_value(), Eq(true));
    if (i % 2 == 0) {
      ASSERT_THAT(iter.deleted(), Eq(true));
    } else {
      ASSERT_THAT(iter.deleted(), Eq(false));
      ByteArray170 value;
      ASSERT_THAT(iter.value(), Eq(value.ToBytes()));
    }
    iter.advance();
  }
  ASSERT_THAT(iter.has_value(), Eq(false));
}

TEST(BlockTest, AddLargeValuesAndDeleteEvery2ndWhileIterating) {
  const auto num_values = 100;
  const auto required_size =
      (Block::kSizeOfValueSizeField + ByteArray21845::kSize) * num_values;
  byte data[required_size];
  Block block(data, sizeof data);
  for (auto i = 0; i != num_values; ++i) {
    ByteArray21845 value;
    ASSERT_THAT(block.Add(value.ToBytes()), Eq(true));
  }

  auto iter = block.NewIterator();
  for (auto i = 0; i != num_values; ++i) {
    ASSERT_THAT(iter.has_value(), Eq(true));
    if (i % 2 == 0) {
      iter.set_deleted();
    }
    iter.advance();
  }
  ASSERT_THAT(iter.has_value(), Eq(false));

  iter = block.NewIterator();
  for (auto i = 0; i != num_values; ++i) {
    ASSERT_THAT(iter.has_value(), Eq(true));
    if (i % 2 == 0) {
      ASSERT_THAT(iter.deleted(), Eq(true));
    } else {
      ASSERT_THAT(iter.deleted(), Eq(false));
      ByteArray21845 value;
      ASSERT_THAT(iter.value(), Eq(value.ToBytes()));
    }
    iter.advance();
  }
  ASSERT_THAT(iter.has_value(), Eq(false));
}

TEST(SuperBlockTest, IsDefaultConstructible) {
  ASSERT_THAT(std::is_default_constructible<SuperBlock>::value, Eq(true));
}

TEST(SuperBlockTest, DefaultConstructedState) {
  ASSERT_THAT(SuperBlock().block_size, Eq(0));
}

TEST(SuperBlockTest, IsCopyConstructibleAndAssignable) {
  ASSERT_THAT(std::is_copy_constructible<SuperBlock>::value, Eq(true));
  ASSERT_THAT(std::is_copy_assignable<SuperBlock>::value, Eq(true));
}

TEST(SuperBlockTest, IsMoveConstructibleAndAssignable) {
  ASSERT_THAT(std::is_move_constructible<SuperBlock>::value, Eq(true));
  ASSERT_THAT(std::is_move_assignable<SuperBlock>::value, Eq(true));
}

TEST(SuperBlockTest, WriteToAndReadFromFileDescriptor) {
  SuperBlock block;
  block.block_size = 12;
  block.major_version = 34;
  block.minor_version = 56;
  const auto tempfile = System::Tempfile();
  block.WriteToFd(tempfile.second);
  ASSERT_THAT(System::Offset(tempfile.second), Eq(SuperBlock::kSerializedSize));
  System::Seek(tempfile.second, 0);
  ASSERT_THAT(System::Offset(tempfile.second), Eq(0));
  ASSERT_THAT(SuperBlock::ReadFromFd(tempfile.second), Eq(block));
  System::Close(tempfile.second);
  boost::filesystem::remove(tempfile.first);
}

}  // namespace internal
}  // namespace multimap
