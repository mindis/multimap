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

  byte* b4end = b4end;
  byte* b32end = b32end;
};

TEST(VarintTest, IsNotDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<Varint>::value);
}

TEST_F(VarintTestFixture, WriteMinN1Value) {
  ASSERT_EQ(1, Varint::writeToBuffer(b4, b4end, Varint::Limits::MIN_N1));
  ASSERT_THAT(b4, ElementsAre(0x00, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinN2Value) {
  ASSERT_EQ(2, Varint::writeToBuffer(b4, b4end, Varint::Limits::MIN_N2));
  ASSERT_THAT(b4, ElementsAre(0x40, 0x40, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinN3Value) {
  ASSERT_EQ(3, Varint::writeToBuffer(b4, b4end, Varint::Limits::MIN_N3));
  ASSERT_THAT(b4, ElementsAre(0x80, 0x40, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinN4Value) {
  ASSERT_EQ(4, Varint::writeToBuffer(b4, b4end, Varint::Limits::MIN_N4));
  ASSERT_THAT(b4, ElementsAre(0xC0, 0x40, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN1Value) {
  ASSERT_EQ(1, Varint::writeToBuffer(b4, b4end, Varint::Limits::MAX_N1));
  ASSERT_THAT(b4, ElementsAre(0x3F, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN2Value) {
  ASSERT_EQ(2, Varint::writeToBuffer(b4, b4end, Varint::Limits::MAX_N2));
  ASSERT_THAT(b4, ElementsAre(0x7F, 0xFF, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN3Value) {
  ASSERT_EQ(3, Varint::writeToBuffer(b4, b4end, Varint::Limits::MAX_N3));
  ASSERT_THAT(b4, ElementsAre(0xBF, 0xFF, 0xFF, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN4Value) {
  ASSERT_EQ(4, Varint::writeToBuffer(b4, b4end, Varint::Limits::MAX_N4));
  ASSERT_THAT(b4, ElementsAre(0xFF, 0xFF, 0xFF, 0xFF));
}

TEST_F(VarintTestFixture, WriteTooBigValueThrows) {
  const auto too_big_value = Varint::Limits::MAX_N4 + 1;
  ASSERT_THROW(Varint::writeToBuffer(b4, b4end, too_big_value),
               std::runtime_error);
}

TEST_F(VarintTestFixture, WriteMinN1ValueWithTrueFlag) {
  ASSERT_EQ(1, Varint::writeToBuffer(b4, b4end,
                                     Varint::Limits::MIN_N1_WITH_FLAG, true));
  ASSERT_THAT(b4, ElementsAre(0x20, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinN1ValueWithFalseFlag) {
  ASSERT_EQ(1, Varint::writeToBuffer(b4, b4end,
                                     Varint::Limits::MIN_N1_WITH_FLAG, false));
  ASSERT_THAT(b4, ElementsAre(0x00, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinN2ValueWithTrueFlag) {
  ASSERT_EQ(2, Varint::writeToBuffer(b4, b4end,
                                     Varint::Limits::MIN_N2_WITH_FLAG, true));
  ASSERT_THAT(b4, ElementsAre(0x60, 0x20, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinN2ValueWithFalseFlag) {
  ASSERT_EQ(2, Varint::writeToBuffer(b4, b4end,
                                     Varint::Limits::MIN_N2_WITH_FLAG, false));
  ASSERT_THAT(b4, ElementsAre(0x40, 0x20, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinN3ValueWithTrueFlag) {
  ASSERT_EQ(3, Varint::writeToBuffer(b4, b4end,
                                     Varint::Limits::MIN_N3_WITH_FLAG, true));
  ASSERT_THAT(b4, ElementsAre(0xA0, 0x20, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinN3ValueWithFalseFlag) {
  ASSERT_EQ(3, Varint::writeToBuffer(b4, b4end,
                                     Varint::Limits::MIN_N3_WITH_FLAG, false));
  ASSERT_THAT(b4, ElementsAre(0x80, 0x20, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinN4ValueWithTrueFlag) {
  ASSERT_EQ(4, Varint::writeToBuffer(b4, b4end,
                                     Varint::Limits::MIN_N4_WITH_FLAG, true));
  ASSERT_THAT(b4, ElementsAre(0xE0, 0x20, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinN4ValueWithFalseFlag) {
  ASSERT_EQ(4, Varint::writeToBuffer(b4, b4end,
                                     Varint::Limits::MIN_N4_WITH_FLAG, false));
  ASSERT_THAT(b4, ElementsAre(0xC0, 0x20, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN1ValueWithTrueFlag) {
  ASSERT_EQ(1, Varint::writeToBuffer(b4, b4end,
                                     Varint::Limits::MAX_N1_WITH_FLAG, true));
  ASSERT_THAT(b4, ElementsAre(0x3F, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN1ValueWithFalseFlag) {
  ASSERT_EQ(1, Varint::writeToBuffer(b4, b4end,
                                     Varint::Limits::MAX_N1_WITH_FLAG, false));
  ASSERT_THAT(b4, ElementsAre(0x1F, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN2ValueWithTrueFlag) {
  ASSERT_EQ(2, Varint::writeToBuffer(b4, b4end,
                                     Varint::Limits::MAX_N2_WITH_FLAG, true));
  ASSERT_THAT(b4, ElementsAre(0x7F, 0xFF, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN2ValueWithFalseFlag) {
  ASSERT_EQ(2, Varint::writeToBuffer(b4, b4end,
                                     Varint::Limits::MAX_N2_WITH_FLAG, false));
  ASSERT_THAT(b4, ElementsAre(0x5F, 0xFF, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN3ValueWithTrueFlag) {
  ASSERT_EQ(3, Varint::writeToBuffer(b4, b4end,
                                     Varint::Limits::MAX_N3_WITH_FLAG, true));
  ASSERT_THAT(b4, ElementsAre(0xBF, 0xFF, 0xFF, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN3ValueWithFalseFlag) {
  ASSERT_EQ(3, Varint::writeToBuffer(b4, b4end,
                                     Varint::Limits::MAX_N3_WITH_FLAG, false));
  ASSERT_THAT(b4, ElementsAre(0x9F, 0xFF, 0xFF, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN4ValueWithTrueFlag) {
  ASSERT_EQ(4, Varint::writeToBuffer(b4, b4end,
                                     Varint::Limits::MAX_N4_WITH_FLAG, true));
  ASSERT_THAT(b4, ElementsAre(0xFF, 0xFF, 0xFF, 0xFF));
}

TEST_F(VarintTestFixture, WriteMaxN4ValueWithFalseFlag) {
  ASSERT_EQ(4, Varint::writeToBuffer(b4, b4end,
                                     Varint::Limits::MAX_N4_WITH_FLAG, false));
  ASSERT_THAT(b4, ElementsAre(0xDF, 0xFF, 0xFF, 0xFF));
}

TEST_F(VarintTestFixture, WriteTooBigValueWithTrueFlagThrows) {
  const uint32_t too_big_value = Varint::Limits::MAX_N4 + 1;
  ASSERT_THROW(Varint::writeToBuffer(b4, b4end, too_big_value, true),
               std::runtime_error);
}

TEST_F(VarintTestFixture, WriteTooBigValueWithFalseFlagThrows) {
  const uint32_t too_big_value = Varint::Limits::MAX_N4 + 1;
  ASSERT_THROW(Varint::writeToBuffer(b4, b4end, too_big_value, false),
               std::runtime_error);
}

TEST_F(VarintTestFixture, ReadMinN1Value) {
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MIN_N1);
  ASSERT_EQ(1, Varint::readFromBuffer(b4, &value));
  ASSERT_EQ(Varint::Limits::MIN_N1, value);
}

TEST_F(VarintTestFixture, ReadMinN2Value) {
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MIN_N2);
  ASSERT_EQ(2, Varint::readFromBuffer(b4, &value));
  ASSERT_EQ(Varint::Limits::MIN_N2, value);
}

TEST_F(VarintTestFixture, ReadMinN3Value) {
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MIN_N3);
  ASSERT_EQ(3, Varint::readFromBuffer(b4, &value));
  ASSERT_EQ(Varint::Limits::MIN_N3, value);
}

TEST_F(VarintTestFixture, ReadMinN4Value) {
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MIN_N4);
  ASSERT_EQ(4, Varint::readFromBuffer(b4, &value));
  ASSERT_EQ(Varint::Limits::MIN_N4, value);
}

TEST_F(VarintTestFixture, ReadMaxN1Value) {
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MAX_N1);
  ASSERT_EQ(1, Varint::readFromBuffer(b4, &value));
  ASSERT_EQ(Varint::Limits::MAX_N1, value);
}

TEST_F(VarintTestFixture, ReadMaxN2Value) {
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MAX_N2);
  ASSERT_EQ(2, Varint::readFromBuffer(b4, &value));
  ASSERT_EQ(Varint::Limits::MAX_N2, value);
}

TEST_F(VarintTestFixture, ReadMaxN3Value) {
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MAX_N3);
  ASSERT_EQ(3, Varint::readFromBuffer(b4, &value));
  ASSERT_EQ(Varint::Limits::MAX_N3, value);
}

TEST_F(VarintTestFixture, ReadMaxN4Value) {
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MAX_N4);
  ASSERT_EQ(4, Varint::readFromBuffer(b4, &value));
  ASSERT_EQ(Varint::Limits::MAX_N4, value);
}

TEST_F(VarintTestFixture, ReadMinN1ValueWithTrueFlag) {
  bool flag = false;
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MIN_N1_WITH_FLAG, true);
  ASSERT_EQ(1, Varint::readFromBuffer(b4, &value, &flag));
  ASSERT_EQ(Varint::Limits::MIN_N1_WITH_FLAG, value);
  ASSERT_TRUE(flag);
}

TEST_F(VarintTestFixture, ReadMinN1ValueWithFalseFlag) {
  bool flag = true;
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MIN_N1_WITH_FLAG, false);
  ASSERT_EQ(1, Varint::readFromBuffer(b4, &value, &flag));
  ASSERT_EQ(Varint::Limits::MIN_N1_WITH_FLAG, value);
  ASSERT_FALSE(flag);
}

TEST_F(VarintTestFixture, ReadMinN2ValueWithTrueFlag) {
  bool flag = false;
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MIN_N2_WITH_FLAG, true);
  ASSERT_EQ(2, Varint::readFromBuffer(b4, &value, &flag));
  ASSERT_EQ(Varint::Limits::MIN_N2_WITH_FLAG, value);
  ASSERT_TRUE(flag);
}

TEST_F(VarintTestFixture, ReadMinN2ValueWithFalseFlag) {
  bool flag = true;
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MIN_N2_WITH_FLAG, false);
  ASSERT_EQ(2, Varint::readFromBuffer(b4, &value, &flag));
  ASSERT_EQ(Varint::Limits::MIN_N2_WITH_FLAG, value);
  ASSERT_FALSE(flag);
}

TEST_F(VarintTestFixture, ReadMinN3ValueWithTrueFlag) {
  bool flag = false;
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MIN_N3_WITH_FLAG, true);
  ASSERT_EQ(3, Varint::readFromBuffer(b4, &value, &flag));
  ASSERT_EQ(Varint::Limits::MIN_N3_WITH_FLAG, value);
  ASSERT_TRUE(flag);
}

TEST_F(VarintTestFixture, ReadMinN3ValueWithFalseFlag) {
  bool flag = true;
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MIN_N3_WITH_FLAG, false);
  ASSERT_EQ(3, Varint::readFromBuffer(b4, &value, &flag));
  ASSERT_EQ(Varint::Limits::MIN_N3_WITH_FLAG, value);
  ASSERT_FALSE(flag);
}

TEST_F(VarintTestFixture, ReadMinN4ValueWithTrueFlag) {
  bool flag = false;
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MIN_N4_WITH_FLAG, true);
  ASSERT_EQ(4, Varint::readFromBuffer(b4, &value, &flag));
  ASSERT_EQ(Varint::Limits::MIN_N4_WITH_FLAG, value);
  ASSERT_TRUE(flag);
}

TEST_F(VarintTestFixture, ReadMinN4ValueWithFalseFlag) {
  bool flag = true;
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MIN_N4_WITH_FLAG, false);
  ASSERT_EQ(4, Varint::readFromBuffer(b4, &value, &flag));
  ASSERT_EQ(Varint::Limits::MIN_N4_WITH_FLAG, value);
  ASSERT_FALSE(flag);
}

TEST_F(VarintTestFixture, ReadMaxN1ValueWithTrueFlag) {
  bool flag = false;
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MAX_N1_WITH_FLAG, true);
  ASSERT_EQ(1, Varint::readFromBuffer(b4, &value, &flag));
  ASSERT_EQ(Varint::Limits::MAX_N1_WITH_FLAG, value);
  ASSERT_TRUE(flag);
}

TEST_F(VarintTestFixture, ReadMaxN1ValueWithFalseFlag) {
  bool flag = true;
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MAX_N1_WITH_FLAG, false);
  ASSERT_EQ(1, Varint::readFromBuffer(b4, &value, &flag));
  ASSERT_EQ(Varint::Limits::MAX_N1_WITH_FLAG, value);
  ASSERT_FALSE(flag);
}

TEST_F(VarintTestFixture, ReadMaxN2ValueWithTrueFlag) {
  bool flag = false;
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MAX_N2_WITH_FLAG, true);
  ASSERT_EQ(2, Varint::readFromBuffer(b4, &value, &flag));
  ASSERT_EQ(Varint::Limits::MAX_N2_WITH_FLAG, value);
  ASSERT_TRUE(flag);
}

TEST_F(VarintTestFixture, ReadMaxN2ValueWithFalseFlag) {
  bool flag = true;
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MAX_N2_WITH_FLAG, false);
  ASSERT_EQ(2, Varint::readFromBuffer(b4, &value, &flag));
  ASSERT_EQ(Varint::Limits::MAX_N2_WITH_FLAG, value);
  ASSERT_FALSE(flag);
}

TEST_F(VarintTestFixture, ReadMaxN3ValueWithTrueFlag) {
  bool flag = false;
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MAX_N3_WITH_FLAG, true);
  ASSERT_EQ(3, Varint::readFromBuffer(b4, &value, &flag));
  ASSERT_EQ(Varint::Limits::MAX_N3_WITH_FLAG, value);
  ASSERT_TRUE(flag);
}

TEST_F(VarintTestFixture, ReadMaxN3ValueWithFalseFlag) {
  bool flag = true;
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MAX_N3_WITH_FLAG, false);
  ASSERT_EQ(3, Varint::readFromBuffer(b4, &value, &flag));
  ASSERT_EQ(Varint::Limits::MAX_N3_WITH_FLAG, value);
  ASSERT_FALSE(flag);
}

TEST_F(VarintTestFixture, ReadMaxN4ValueWithTrueFlag) {
  bool flag = false;
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MAX_N4_WITH_FLAG, true);
  ASSERT_EQ(4, Varint::readFromBuffer(b4, &value, &flag));
  ASSERT_EQ(Varint::Limits::MAX_N4_WITH_FLAG, value);
  ASSERT_TRUE(flag);
}

TEST_F(VarintTestFixture, ReadMaxN4ValueWithFalseFlag) {
  bool flag = true;
  uint32_t value = 0;
  Varint::writeToBuffer(b4, b4end, Varint::Limits::MAX_N4_WITH_FLAG, false);
  ASSERT_EQ(4, Varint::readFromBuffer(b4, &value, &flag));
  ASSERT_EQ(Varint::Limits::MAX_N4_WITH_FLAG, value);
  ASSERT_FALSE(flag);
}

TEST_F(VarintTestFixture, WriteAndReadSequenceOfValues) {
  // clang-format off
  const uint32_t v1 = (Varint::Limits::MAX_N1 - Varint::Limits::MIN_N1) / 2;
  const uint32_t v2 = (Varint::Limits::MAX_N2 - Varint::Limits::MIN_N2) / 2;
  const uint32_t v3 = (Varint::Limits::MAX_N3 - Varint::Limits::MIN_N3) / 2;
  const uint32_t v4 = (Varint::Limits::MAX_N4 - Varint::Limits::MIN_N4) / 2;

  size_t off = 0;
  byte* pos = b32;
  ASSERT_EQ(1, Varint::writeToBuffer(pos + off, b32end, v1)); pos += 1;
  ASSERT_EQ(2, Varint::writeToBuffer(pos + off, b32end, v2)); pos += 2;
  ASSERT_EQ(3, Varint::writeToBuffer(pos + off, b32end, v3)); pos += 3;
  ASSERT_EQ(4, Varint::writeToBuffer(pos + off, b32end, v4)); pos += 4;

  pos = b32;
  uint32_t value = 0;
  ASSERT_EQ(1, Varint::readFromBuffer(pos, &value)); pos += 1;
  ASSERT_EQ(2, Varint::readFromBuffer(pos, &value)); pos += 2;
  ASSERT_EQ(3, Varint::readFromBuffer(pos, &value)); pos += 3;
  ASSERT_EQ(4, Varint::readFromBuffer(pos, &value)); pos += 4;
  // clang-format on
}

TEST_F(VarintTestFixture, WriteAndReadSequenceOfValuesWithTrueFlags) {
  // clang-format off
  using VL = Varint::Limits;
  const uint32_t v1 = (VL::MAX_N1_WITH_FLAG - VL::MIN_N1_WITH_FLAG) / 2;
  const uint32_t v2 = (VL::MAX_N2_WITH_FLAG - VL::MIN_N2_WITH_FLAG) / 2;
  const uint32_t v3 = (VL::MAX_N3_WITH_FLAG - VL::MIN_N3_WITH_FLAG) / 2;
  const uint32_t v4 = (VL::MAX_N4_WITH_FLAG - VL::MIN_N4_WITH_FLAG) / 2;

  size_t off = 0;
  byte* pos = b32;
  bool flag = true;
  ASSERT_EQ(1, Varint::writeToBuffer(pos + off, b32end, v1, flag)); pos += 1;
  ASSERT_EQ(2, Varint::writeToBuffer(pos + off, b32end, v2, flag)); pos += 2;
  ASSERT_EQ(3, Varint::writeToBuffer(pos + off, b32end, v3, flag)); pos += 3;
  ASSERT_EQ(4, Varint::writeToBuffer(pos + off, b32end, v4, flag)); pos += 4;

  pos = b32;
  flag = false;
  uint32_t value = 0;
  ASSERT_EQ(1, Varint::readFromBuffer(pos, &value, &flag)); pos += 1;
  ASSERT_EQ(v1, value);
  ASSERT_TRUE(flag);

  flag = false;
  ASSERT_EQ(2, Varint::readFromBuffer(pos, &value, &flag)); pos += 2;
  ASSERT_EQ(v2, value);
  ASSERT_TRUE(flag);

  flag = false;
  ASSERT_EQ(3, Varint::readFromBuffer(pos, &value, &flag)); pos += 3;
  ASSERT_EQ(v3, value);
  ASSERT_TRUE(flag);

  flag = false;
  ASSERT_EQ(4, Varint::readFromBuffer(pos, &value, &flag)); pos += 4;
  ASSERT_EQ(v4, value);
  ASSERT_TRUE(flag);
  // clang-format on
}

TEST_F(VarintTestFixture, WriteAndReadSequenceOfValuesWithFalseFlags) {
  // clang-format off
  using VL = Varint::Limits;
  const uint32_t v1 = (VL::MAX_N1_WITH_FLAG - VL::MIN_N1_WITH_FLAG) / 2;
  const uint32_t v2 = (VL::MAX_N2_WITH_FLAG - VL::MIN_N2_WITH_FLAG) / 2;
  const uint32_t v3 = (VL::MAX_N3_WITH_FLAG - VL::MIN_N3_WITH_FLAG) / 2;
  const uint32_t v4 = (VL::MAX_N4_WITH_FLAG - VL::MIN_N4_WITH_FLAG) / 2;

  size_t off = 0;
  byte* pos = b32;
  bool flag = false;
  ASSERT_EQ(1, Varint::writeToBuffer(pos + off, b32end, v1, flag)); pos += 1;
  ASSERT_EQ(2, Varint::writeToBuffer(pos + off, b32end, v2, flag)); pos += 2;
  ASSERT_EQ(3, Varint::writeToBuffer(pos + off, b32end, v3, flag)); pos += 3;
  ASSERT_EQ(4, Varint::writeToBuffer(pos + off, b32end, v4, flag)); pos += 4;

  pos = b32;
  flag = true;
  uint32_t value = 0;
  ASSERT_EQ(1, Varint::readFromBuffer(pos, &value, &flag)); pos += 1;
  ASSERT_EQ(v1, value);
  ASSERT_FALSE(flag);

  flag = true;
  ASSERT_EQ(2, Varint::readFromBuffer(pos, &value, &flag)); pos += 2;
  ASSERT_EQ(v2, value);
  ASSERT_FALSE(flag);

  flag = true;
  ASSERT_EQ(3, Varint::readFromBuffer(pos, &value, &flag)); pos += 3;
  ASSERT_EQ(v3, value);
  ASSERT_FALSE(flag);

  flag = true;
  ASSERT_EQ(4, Varint::readFromBuffer(pos, &value, &flag)); pos += 4;
  ASSERT_EQ(v4, value);
  ASSERT_FALSE(flag);
  // clang-format on
}

TEST_F(VarintTestFixture, WriteAndReadSequenceOfValuesWithTrueAndFalseFlags) {
  // clang-format off
  using VL = Varint::Limits;
  const uint32_t v1 = (VL::MAX_N1_WITH_FLAG - VL::MIN_N1_WITH_FLAG) / 2;
  const uint32_t v2 = (VL::MAX_N2_WITH_FLAG - VL::MIN_N2_WITH_FLAG) / 2;
  const uint32_t v3 = (VL::MAX_N3_WITH_FLAG - VL::MIN_N3_WITH_FLAG) / 2;
  const uint32_t v4 = (VL::MAX_N4_WITH_FLAG - VL::MIN_N4_WITH_FLAG) / 2;

  size_t off = 0;
  byte* pos = b32;
  ASSERT_EQ(1, Varint::writeToBuffer(pos + off, b32end, v1, true));  pos += 1;
  ASSERT_EQ(2, Varint::writeToBuffer(pos + off, b32end, v2, false)); pos += 2;
  ASSERT_EQ(3, Varint::writeToBuffer(pos + off, b32end, v3, true));  pos += 3;
  ASSERT_EQ(4, Varint::writeToBuffer(pos + off, b32end, v4, false)); pos += 4;

  pos = b32;
  bool flag = false;
  uint32_t value = 0;
  ASSERT_EQ(1, Varint::readFromBuffer(pos, &value, &flag)); pos += 1;
  ASSERT_EQ(v1, value);
  ASSERT_TRUE(flag);

  ASSERT_EQ(2, Varint::readFromBuffer(pos, &value, &flag)); pos += 2;
  ASSERT_EQ(v2, value);
  ASSERT_FALSE(flag);

  ASSERT_EQ(3, Varint::readFromBuffer(pos, &value, &flag)); pos += 3;
  ASSERT_EQ(v3, value);
  ASSERT_TRUE(flag);

  ASSERT_EQ(4, Varint::readFromBuffer(pos, &value, &flag)); pos += 4;
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
