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

#include <cstring>
#include <type_traits>
#include "gmock/gmock.h"
#include "multimap/internal/Varint.hpp"

namespace multimap {
namespace internal {

using testing::Eq;
using testing::ElementsAre;

struct VarintTestFixture : public testing::Test {
  void SetUp() override {
    std::memset(b4, 0, sizeof b4);
    std::memset(b32, 0, sizeof b32);
  }

  byte b4[4];
  byte b32[32];

  byte* b4end = std::end(b4);
  byte* b32end = std::end(b32);
};

TEST(VarintTest, IsNotDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<Varint>::value);
}

TEST_F(VarintTestFixture, WriteMinVarintWithTrueFlag) {
  ASSERT_EQ(1, Varint::writeToBuffer(b4, b4end, 0, true));
  ASSERT_THAT(b4, ElementsAre(0x20, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinVarintWithFalseFlag) {
  ASSERT_EQ(1, Varint::writeToBuffer(b4, b4end, 0, false));
  ASSERT_THAT(b4, ElementsAre(0x00, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxVarintWithTrueFlag) {
  ASSERT_EQ(
      4, Varint::writeToBuffer(b4, b4end, Varint::MAX_VARINT_WITH_FLAG, true));
  ASSERT_THAT(b4, ElementsAre(0xFF, 0xFF, 0xFF, 0xFF));
}

TEST_F(VarintTestFixture, WriteMaxVarintWithFalseFlag) {
  ASSERT_EQ(
      4, Varint::writeToBuffer(b4, b4end, Varint::MAX_VARINT_WITH_FLAG, false));
  ASSERT_THAT(b4, ElementsAre(0xDF, 0xFF, 0xFF, 0xFF));
}

TEST_F(VarintTestFixture, WriteTooBigValueWithTrueFlagThrows) {
  const uint32_t too_big_value = Varint::MAX_VARINT_WITH_FLAG + 1;
  ASSERT_THROW(Varint::writeToBuffer(b4, b4end, too_big_value, true),
               std::runtime_error);
}

TEST_F(VarintTestFixture, WriteTooBigValueWithFalseFlagThrows) {
  const uint32_t too_big_value = Varint::MAX_VARINT_WITH_FLAG + 1;
  ASSERT_THROW(Varint::writeToBuffer(b4, b4end, too_big_value, false),
               std::runtime_error);
}

TEST_F(VarintTestFixture, ReadMinVarintWithTrueFlag) {
  Varint::writeToBuffer(b4, b4end, 0, true);
  bool flag = false;
  uint32_t value = -1;
  ASSERT_EQ(1, Varint::readFromBuffer(b4, b4end, &value, &flag));
  ASSERT_EQ(0, value);
  ASSERT_TRUE(flag);
}

TEST_F(VarintTestFixture, ReadMinVarintWithFalseFlag) {
  Varint::writeToBuffer(b4, b4end, 0, false);
  bool flag = true;
  uint32_t value = -1;
  ASSERT_EQ(1, Varint::readFromBuffer(b4, b4end, &value, &flag));
  ASSERT_EQ(0, value);
  ASSERT_FALSE(flag);
}

TEST_F(VarintTestFixture, ReadMaxVarintWithTrueFlag) {
  Varint::writeToBuffer(b4, b4end, Varint::MAX_VARINT_WITH_FLAG, true);
  bool flag = false;
  uint32_t value = 0;
  ASSERT_EQ(4, Varint::readFromBuffer(b4, b4end, &value, &flag));
  ASSERT_EQ(Varint::MAX_VARINT_WITH_FLAG, value);
  ASSERT_TRUE(flag);
}

TEST_F(VarintTestFixture, ReadMaxVarintWithFalseFlag) {
  Varint::writeToBuffer(b4, b4end, Varint::MAX_VARINT_WITH_FLAG, false);
  bool flag = true;
  uint32_t value = 0;
  ASSERT_EQ(4, Varint::readFromBuffer(b4, b4end, &value, &flag));
  ASSERT_EQ(Varint::MAX_VARINT_WITH_FLAG, value);
  ASSERT_FALSE(flag);
}

TEST_F(VarintTestFixture, WriteAndReadMultipleVarintsWithTrueFlags) {
  // clang-format off
  const uint32_t v1 = 23;
  const uint32_t v2 = 23 << 7;
  const uint32_t v3 = 23 << 14;
  const uint32_t v4 = 23 << 21;

  byte* pos = b32;
  bool flag = true;
  ASSERT_EQ(1, Varint::writeToBuffer(pos, b32end, v1, flag)); pos += 1;
  ASSERT_EQ(2, Varint::writeToBuffer(pos, b32end, v2, flag)); pos += 2;
  ASSERT_EQ(3, Varint::writeToBuffer(pos, b32end, v3, flag)); pos += 3;
  ASSERT_EQ(4, Varint::writeToBuffer(pos, b32end, v4, flag)); pos += 4;

  pos = b32;
  flag = false;
  uint32_t value = 0;
  ASSERT_EQ(1, Varint::readFromBuffer(pos, b32end, &value, &flag)); pos += 1;
  ASSERT_EQ(v1, value);
  ASSERT_TRUE(flag);

  flag = false;
  ASSERT_EQ(2, Varint::readFromBuffer(pos, b32end, &value, &flag)); pos += 2;
  ASSERT_EQ(v2, value);
  ASSERT_TRUE(flag);

  flag = false;
  ASSERT_EQ(3, Varint::readFromBuffer(pos, b32end, &value, &flag)); pos += 3;
  ASSERT_EQ(v3, value);
  ASSERT_TRUE(flag);

  flag = false;
  ASSERT_EQ(4, Varint::readFromBuffer(pos, b32end, &value, &flag)); pos += 4;
  ASSERT_EQ(v4, value);
  ASSERT_TRUE(flag);
  // clang-format on
}

TEST_F(VarintTestFixture, WriteAndReadMultipleVarintsWithFalseFlags) {
  // clang-format off
  const uint32_t v1 = 23;
  const uint32_t v2 = 23 << 7;
  const uint32_t v3 = 23 << 14;
  const uint32_t v4 = 23 << 21;

  byte* pos = b32;
  bool flag = false;
  ASSERT_EQ(1, Varint::writeToBuffer(pos, b32end, v1, flag)); pos += 1;
  ASSERT_EQ(2, Varint::writeToBuffer(pos, b32end, v2, flag)); pos += 2;
  ASSERT_EQ(3, Varint::writeToBuffer(pos, b32end, v3, flag)); pos += 3;
  ASSERT_EQ(4, Varint::writeToBuffer(pos, b32end, v4, flag)); pos += 4;

  pos = b32;
  flag = true;
  uint32_t value = 0;
  ASSERT_EQ(1, Varint::readFromBuffer(pos, b32end, &value, &flag)); pos += 1;
  ASSERT_EQ(v1, value);
  ASSERT_FALSE(flag);

  flag = true;
  ASSERT_EQ(2, Varint::readFromBuffer(pos, b32end, &value, &flag)); pos += 2;
  ASSERT_EQ(v2, value);
  ASSERT_FALSE(flag);

  flag = true;
  ASSERT_EQ(3, Varint::readFromBuffer(pos, b32end, &value, &flag)); pos += 3;
  ASSERT_EQ(v3, value);
  ASSERT_FALSE(flag);

  flag = true;
  ASSERT_EQ(4, Varint::readFromBuffer(pos, b32end, &value, &flag)); pos += 4;
  ASSERT_EQ(v4, value);
  ASSERT_FALSE(flag);
  // clang-format on
}

TEST_F(VarintTestFixture, WriteAndReadMultipleVarintsWithTrueAndFalseFlags) {
  // clang-format off
  const uint32_t v1 = 23;
  const uint32_t v2 = 23 << 7;
  const uint32_t v3 = 23 << 14;
  const uint32_t v4 = 23 << 21;

  byte* pos = b32;
  ASSERT_EQ(1, Varint::writeToBuffer(pos, b32end, v1, true));  pos += 1;
  ASSERT_EQ(2, Varint::writeToBuffer(pos, b32end, v2, false)); pos += 2;
  ASSERT_EQ(3, Varint::writeToBuffer(pos, b32end, v3, true));  pos += 3;
  ASSERT_EQ(4, Varint::writeToBuffer(pos, b32end, v4, false)); pos += 4;

  pos = b32;
  bool flag = false;
  uint32_t value = 0;
  ASSERT_EQ(1, Varint::readFromBuffer(pos, b32end, &value, &flag)); pos += 1;
  ASSERT_EQ(v1, value);
  ASSERT_TRUE(flag);

  ASSERT_EQ(2, Varint::readFromBuffer(pos, b32end, &value, &flag)); pos += 2;
  ASSERT_EQ(v2, value);
  ASSERT_FALSE(flag);

  ASSERT_EQ(3, Varint::readFromBuffer(pos, b32end, &value, &flag)); pos += 3;
  ASSERT_EQ(v3, value);
  ASSERT_TRUE(flag);

  ASSERT_EQ(4, Varint::readFromBuffer(pos, b32end, &value, &flag)); pos += 4;
  ASSERT_EQ(v4, value);
  ASSERT_FALSE(flag);
  // clang-format on
}

TEST_F(VarintTestFixture, WriteTrueFlag) {
  std::memset(b4, 0xDF, sizeof b4);
  Varint::setFlagInBuffer(b4, true);
  ASSERT_THAT(b4, ElementsAre(0xFF, 0xDF, 0xDF, 0xDF));
}

TEST_F(VarintTestFixture, WriteFalseFlag) {
  std::memset(b4, 0xFF, sizeof b4);
  Varint::setFlagInBuffer(b4, false);
  ASSERT_THAT(b4, ElementsAre(0xDF, 0xFF, 0xFF, 0xFF));
}

}  // namespace internal
}  // namespace multimap
