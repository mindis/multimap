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
#include <limits>
#include <type_traits>
#include "gmock/gmock.h"
#include "multimap/internal/Varint.h"

namespace multimap {
namespace internal {

using testing::Eq;
using testing::ElementsAre;

struct VarintTestFixture : public testing::Test {
  void SetUp() override {
    std::memset(b5, 0, sizeof b5);
    std::memset(b25, 0, sizeof b25);
  }

  byte b5[5];
  byte b25[25];

  byte* b5end = std::end(b5);
  byte* b25end = std::end(b25);
};

TEST(VarintTest, IsNotDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<Varint>::value);
}

TEST_F(VarintTestFixture, WriteMinVarintWithTrueFlag) {
  ASSERT_EQ(1, Varint::writeToBuffer(b5, b5end, 0, true));
  ASSERT_THAT(b5, ElementsAre(0x40, 0x00, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinVarintWithFalseFlag) {
  ASSERT_EQ(1, Varint::writeToBuffer(b5, b5end, 0, false));
  ASSERT_THAT(b5, ElementsAre(0x00, 0x00, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxVarintWithTrueFlag) {
  ASSERT_EQ(5, Varint::writeToBuffer(
                   b5, b5end, std::numeric_limits<uint32_t>::max(), true));
  ASSERT_THAT(b5, ElementsAre(0xFF, 0xFF, 0xFF, 0xFF, 0x1F));
}

TEST_F(VarintTestFixture, WriteMaxVarintWithFalseFlag) {
  ASSERT_EQ(5, Varint::writeToBuffer(
                   b5, b5end, std::numeric_limits<uint32_t>::max(), false));
  ASSERT_THAT(b5, ElementsAre(0xBF, 0xFF, 0xFF, 0xFF, 0x1F));
}

TEST_F(VarintTestFixture, ReadMinVarintWithTrueFlag) {
  Varint::writeToBuffer(b5, b5end, 0, true);
  bool flag = false;
  uint32_t value = -1;
  ASSERT_EQ(1, Varint::readFromBuffer(b5, b5end, &value, &flag));
  ASSERT_EQ(0, value);
  ASSERT_TRUE(flag);
}

TEST_F(VarintTestFixture, ReadMinVarintWithFalseFlag) {
  Varint::writeToBuffer(b5, b5end, 0, false);
  bool flag = true;
  uint32_t value = -1;
  ASSERT_EQ(1, Varint::readFromBuffer(b5, b5end, &value, &flag));
  ASSERT_EQ(0, value);
  ASSERT_FALSE(flag);
}

TEST_F(VarintTestFixture, ReadMaxVarintWithTrueFlag) {
  Varint::writeToBuffer(b5, b5end, std::numeric_limits<uint32_t>::max(), true);
  bool flag = false;
  uint32_t value = 0;
  ASSERT_EQ(5, Varint::readFromBuffer(b5, b5end, &value, &flag));
  ASSERT_EQ(std::numeric_limits<uint32_t>::max(), value);
  ASSERT_TRUE(flag);
}

TEST_F(VarintTestFixture, ReadMaxVarintWithFalseFlag) {
  Varint::writeToBuffer(b5, b5end, std::numeric_limits<uint32_t>::max(), false);
  bool flag = true;
  uint32_t value = 0;
  ASSERT_EQ(5, Varint::readFromBuffer(b5, b5end, &value, &flag));
  ASSERT_EQ(std::numeric_limits<uint32_t>::max(), value);
  ASSERT_FALSE(flag);
}

TEST_F(VarintTestFixture, WriteAndReadMultipleVarintsWithTrueFlags) {
  // clang-format off
  const uint32_t v1 = 23;
  const uint32_t v2 = 23 << 7;
  const uint32_t v3 = 23 << 14;
  const uint32_t v4 = 23 << 21;
  const uint32_t v5 = 23 << 28;

  byte* pos = b25;
  bool flag = true;
  ASSERT_EQ(1, Varint::writeToBuffer(pos, b25end, v1, flag)); pos += 1;
  ASSERT_EQ(2, Varint::writeToBuffer(pos, b25end, v2, flag)); pos += 2;
  ASSERT_EQ(3, Varint::writeToBuffer(pos, b25end, v3, flag)); pos += 3;
  ASSERT_EQ(4, Varint::writeToBuffer(pos, b25end, v4, flag)); pos += 4;
  ASSERT_EQ(5, Varint::writeToBuffer(pos, b25end, v5, flag)); pos += 5;

  pos = b25;
  flag = false;
  uint32_t value = 0;
  ASSERT_EQ(1, Varint::readFromBuffer(pos, b25end, &value, &flag)); pos += 1;
  ASSERT_EQ(v1, value);
  ASSERT_TRUE(flag);

  flag = false;
  ASSERT_EQ(2, Varint::readFromBuffer(pos, b25end, &value, &flag)); pos += 2;
  ASSERT_EQ(v2, value);
  ASSERT_TRUE(flag);

  flag = false;
  ASSERT_EQ(3, Varint::readFromBuffer(pos, b25end, &value, &flag)); pos += 3;
  ASSERT_EQ(v3, value);
  ASSERT_TRUE(flag);

  flag = false;
  ASSERT_EQ(4, Varint::readFromBuffer(pos, b25end, &value, &flag)); pos += 4;
  ASSERT_EQ(v4, value);
  ASSERT_TRUE(flag);

  flag = false;
  ASSERT_EQ(5, Varint::readFromBuffer(pos, b25end, &value, &flag)); pos += 5;
  ASSERT_EQ(v5, value);
  ASSERT_TRUE(flag);
  // clang-format on
}

TEST_F(VarintTestFixture, WriteAndReadMultipleVarintsWithFalseFlags) {
  // clang-format off
  const uint32_t v1 = 23;
  const uint32_t v2 = 23 << 7;
  const uint32_t v3 = 23 << 14;
  const uint32_t v4 = 23 << 21;
  const uint32_t v5 = 23 << 28;

  byte* pos = b25;
  bool flag = false;
  ASSERT_EQ(1, Varint::writeToBuffer(pos, b25end, v1, flag)); pos += 1;
  ASSERT_EQ(2, Varint::writeToBuffer(pos, b25end, v2, flag)); pos += 2;
  ASSERT_EQ(3, Varint::writeToBuffer(pos, b25end, v3, flag)); pos += 3;
  ASSERT_EQ(4, Varint::writeToBuffer(pos, b25end, v4, flag)); pos += 4;
  ASSERT_EQ(5, Varint::writeToBuffer(pos, b25end, v5, flag)); pos += 5;

  pos = b25;
  flag = true;
  uint32_t value = 0;
  ASSERT_EQ(1, Varint::readFromBuffer(pos, b25end, &value, &flag)); pos += 1;
  ASSERT_EQ(v1, value);
  ASSERT_FALSE(flag);

  flag = true;
  ASSERT_EQ(2, Varint::readFromBuffer(pos, b25end, &value, &flag)); pos += 2;
  ASSERT_EQ(v2, value);
  ASSERT_FALSE(flag);

  flag = true;
  ASSERT_EQ(3, Varint::readFromBuffer(pos, b25end, &value, &flag)); pos += 3;
  ASSERT_EQ(v3, value);
  ASSERT_FALSE(flag);

  flag = true;
  ASSERT_EQ(4, Varint::readFromBuffer(pos, b25end, &value, &flag)); pos += 4;
  ASSERT_EQ(v4, value);
  ASSERT_FALSE(flag);

  flag = true;
  ASSERT_EQ(5, Varint::readFromBuffer(pos, b25end, &value, &flag)); pos += 5;
  ASSERT_EQ(v5, value);
  ASSERT_FALSE(flag);
  // clang-format on
}

TEST_F(VarintTestFixture, WriteAndReadMultipleVarintsWithTrueAndFalseFlags) {
  // clang-format off
  const uint32_t v1 = 23;
  const uint32_t v2 = 23 << 7;
  const uint32_t v3 = 23 << 14;
  const uint32_t v4 = 23 << 21;
  const uint32_t v5 = 23 << 28;

  byte* pos = b25;
  ASSERT_EQ(1, Varint::writeToBuffer(pos, b25end, v1, true));  pos += 1;
  ASSERT_EQ(2, Varint::writeToBuffer(pos, b25end, v2, false)); pos += 2;
  ASSERT_EQ(3, Varint::writeToBuffer(pos, b25end, v3, true));  pos += 3;
  ASSERT_EQ(4, Varint::writeToBuffer(pos, b25end, v4, false)); pos += 4;
  ASSERT_EQ(5, Varint::writeToBuffer(pos, b25end, v5, true));  pos += 5;

  pos = b25;
  bool flag = false;
  uint32_t value = 0;
  ASSERT_EQ(1, Varint::readFromBuffer(pos, b25end, &value, &flag)); pos += 1;
  ASSERT_EQ(v1, value);
  ASSERT_TRUE(flag);

  ASSERT_EQ(2, Varint::readFromBuffer(pos, b25end, &value, &flag)); pos += 2;
  ASSERT_EQ(v2, value);
  ASSERT_FALSE(flag);

  ASSERT_EQ(3, Varint::readFromBuffer(pos, b25end, &value, &flag)); pos += 3;
  ASSERT_EQ(v3, value);
  ASSERT_TRUE(flag);

  ASSERT_EQ(4, Varint::readFromBuffer(pos, b25end, &value, &flag)); pos += 4;
  ASSERT_EQ(v4, value);
  ASSERT_FALSE(flag);

  ASSERT_EQ(5, Varint::readFromBuffer(pos, b25end, &value, &flag)); pos += 5;
  ASSERT_EQ(v5, value);
  ASSERT_TRUE(flag);
  // clang-format on
}

TEST_F(VarintTestFixture, WriteTrueFlag) {
  std::memset(b5, 0xBF, sizeof b5);
  Varint::setFlagInBuffer(b5, true);
  ASSERT_THAT(b5, ElementsAre(0xFF, 0xBF, 0xBF, 0xBF, 0xBF));
}

TEST_F(VarintTestFixture, WriteFalseFlag) {
  std::memset(b5, 0xFF, sizeof b5);
  Varint::setFlagInBuffer(b5, false);
  ASSERT_THAT(b5, ElementsAre(0xBF, 0xFF, 0xFF, 0xFF, 0xFF));
}

}  // namespace internal
}  // namespace multimap
