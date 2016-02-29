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

#include <cstring>  // For std::memset
#include <type_traits>
#include "gmock/gmock.h"
#include "multimap/internal/Varint.hpp"
#include "multimap/thirdparty/mt/mt.hpp"

namespace multimap {
namespace internal {

using testing::Eq;
using testing::ElementsAre;

struct VarintTestFixture : public testing::Test {
  void SetUp() override {
    std::memset(b32, 0, sizeof b32);
    std::memset(b4, 0, sizeof b4);
  }

  uint32_t value;
  bool flag;

  char b32[32];
  char b4[4];
};

TEST(VarintTest, IsNotDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<Varint>::value);
}

TEST_F(VarintTestFixture, WriteValueToNullPtrThrows) {
  ASSERT_THROW(Varint::writeUintToBuffer(nullptr, 1, value),
               mt::AssertionError);
}

TEST_F(VarintTestFixture, WriteMinN1Value) {
  ASSERT_THAT(Varint::writeUintToBuffer(b4, sizeof b4, Varint::Limits::MIN_N1),
              Eq(1));
  ASSERT_THAT(b4, ElementsAre(0x00, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinN2Value) {
  ASSERT_THAT(Varint::writeUintToBuffer(b4, sizeof b4, Varint::Limits::MIN_N2),
              Eq(2));
  ASSERT_THAT(b4, ElementsAre(0x40, 0x40, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinN3Value) {
  ASSERT_THAT(Varint::writeUintToBuffer(b4, sizeof b4, Varint::Limits::MIN_N3),
              Eq(3));
  ASSERT_THAT(b4, ElementsAre(0x80, 0x40, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinN4Value) {
  ASSERT_THAT(Varint::writeUintToBuffer(b4, sizeof b4, Varint::Limits::MIN_N4),
              Eq(4));
  ASSERT_THAT(b4, ElementsAre(0xC0, 0x40, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN1Value) {
  ASSERT_THAT(Varint::writeUintToBuffer(b4, sizeof b4, Varint::Limits::MAX_N1),
              Eq(1));
  ASSERT_THAT(b4, ElementsAre(0x3F, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN2Value) {
  ASSERT_THAT(Varint::writeUintToBuffer(b4, sizeof b4, Varint::Limits::MAX_N2),
              Eq(2));
  ASSERT_THAT(b4, ElementsAre(0x7F, 0xFF, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN3Value) {
  ASSERT_THAT(Varint::writeUintToBuffer(b4, sizeof b4, Varint::Limits::MAX_N3),
              Eq(3));
  ASSERT_THAT(b4, ElementsAre(0xBF, 0xFF, 0xFF, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN4Value) {
  ASSERT_THAT(Varint::writeUintToBuffer(b4, sizeof b4, Varint::Limits::MAX_N4),
              Eq(4));
  ASSERT_THAT(b4, ElementsAre(0xFF, 0xFF, 0xFF, 0xFF));
}

TEST_F(VarintTestFixture, WriteTooBigValueThrows) {
  const auto too_big_value = Varint::Limits::MAX_N4 + 1;
  ASSERT_THROW(Varint::writeUintToBuffer(b4, sizeof b4, too_big_value),
               std::runtime_error);
}

TEST_F(VarintTestFixture, WriteMinN1ValueWithTrueFlag) {
  ASSERT_THAT(Varint::writeUintWithFlagToBuffer(
                  b4, sizeof b4, Varint::Limits::MIN_N1_WITH_FLAG, true),
              Eq(1));
  ASSERT_THAT(b4, ElementsAre(0x20, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinN1ValueWithFalseFlag) {
  ASSERT_THAT(Varint::writeUintWithFlagToBuffer(
                  b4, sizeof b4, Varint::Limits::MIN_N1_WITH_FLAG, false),
              Eq(1));
  ASSERT_THAT(b4, ElementsAre(0x00, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinN2ValueWithTrueFlag) {
  ASSERT_THAT(Varint::writeUintWithFlagToBuffer(
                  b4, sizeof b4, Varint::Limits::MIN_N2_WITH_FLAG, true),
              Eq(2));
  ASSERT_THAT(b4, ElementsAre(0x60, 0x20, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinN2ValueWithFalseFlag) {
  ASSERT_THAT(Varint::writeUintWithFlagToBuffer(
                  b4, sizeof b4, Varint::Limits::MIN_N2_WITH_FLAG, false),
              Eq(2));
  ASSERT_THAT(b4, ElementsAre(0x40, 0x20, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinN3ValueWithTrueFlag) {
  ASSERT_THAT(Varint::writeUintWithFlagToBuffer(
                  b4, sizeof b4, Varint::Limits::MIN_N3_WITH_FLAG, true),
              Eq(3));
  ASSERT_THAT(b4, ElementsAre(0xA0, 0x20, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinN3ValueWithFalseFlag) {
  ASSERT_THAT(Varint::writeUintWithFlagToBuffer(
                  b4, sizeof b4, Varint::Limits::MIN_N3_WITH_FLAG, false),
              Eq(3));
  ASSERT_THAT(b4, ElementsAre(0x80, 0x20, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinN4ValueWithTrueFlag) {
  ASSERT_THAT(Varint::writeUintWithFlagToBuffer(
                  b4, sizeof b4, Varint::Limits::MIN_N4_WITH_FLAG, true),
              Eq(4));
  ASSERT_THAT(b4, ElementsAre(0xE0, 0x20, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinN4ValueWithFalseFlag) {
  ASSERT_THAT(Varint::writeUintWithFlagToBuffer(
                  b4, sizeof b4, Varint::Limits::MIN_N4_WITH_FLAG, false),
              Eq(4));
  ASSERT_THAT(b4, ElementsAre(0xC0, 0x20, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN1ValueWithTrueFlag) {
  ASSERT_THAT(Varint::writeUintWithFlagToBuffer(
                  b4, sizeof b4, Varint::Limits::MAX_N1_WITH_FLAG, true),
              Eq(1));
  ASSERT_THAT(b4, ElementsAre(0x3F, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN1ValueWithFalseFlag) {
  ASSERT_THAT(Varint::writeUintWithFlagToBuffer(
                  b4, sizeof b4, Varint::Limits::MAX_N1_WITH_FLAG, false),
              Eq(1));
  ASSERT_THAT(b4, ElementsAre(0x1F, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN2ValueWithTrueFlag) {
  ASSERT_THAT(Varint::writeUintWithFlagToBuffer(
                  b4, sizeof b4, Varint::Limits::MAX_N2_WITH_FLAG, true),
              Eq(2));
  ASSERT_THAT(b4, ElementsAre(0x7F, 0xFF, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN2ValueWithFalseFlag) {
  ASSERT_THAT(Varint::writeUintWithFlagToBuffer(
                  b4, sizeof b4, Varint::Limits::MAX_N2_WITH_FLAG, false),
              Eq(2));
  ASSERT_THAT(b4, ElementsAre(0x5F, 0xFF, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN3ValueWithTrueFlag) {
  ASSERT_THAT(Varint::writeUintWithFlagToBuffer(
                  b4, sizeof b4, Varint::Limits::MAX_N3_WITH_FLAG, true),
              Eq(3));
  ASSERT_THAT(b4, ElementsAre(0xBF, 0xFF, 0xFF, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN3ValueWithFalseFlag) {
  ASSERT_THAT(Varint::writeUintWithFlagToBuffer(
                  b4, sizeof b4, Varint::Limits::MAX_N3_WITH_FLAG, false),
              Eq(3));
  ASSERT_THAT(b4, ElementsAre(0x9F, 0xFF, 0xFF, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxN4ValueWithTrueFlag) {
  ASSERT_THAT(Varint::writeUintWithFlagToBuffer(
                  b4, sizeof b4, Varint::Limits::MAX_N4_WITH_FLAG, true),
              Eq(4));
  ASSERT_THAT(b4, ElementsAre(0xFF, 0xFF, 0xFF, 0xFF));
}

TEST_F(VarintTestFixture, WriteMaxN4ValueWithFalseFlag) {
  ASSERT_THAT(Varint::writeUintWithFlagToBuffer(
                  b4, sizeof b4, Varint::Limits::MAX_N4_WITH_FLAG, false),
              Eq(4));
  ASSERT_THAT(b4, ElementsAre(0xDF, 0xFF, 0xFF, 0xFF));
}

TEST_F(VarintTestFixture, WriteTooBigValueWithTrueFlagThrows) {
  const auto too_big_value = Varint::Limits::MAX_N4 + 1;
  ASSERT_THROW(
      Varint::writeUintWithFlagToBuffer(b4, sizeof b4, too_big_value, true),
      std::runtime_error);
}

TEST_F(VarintTestFixture, WriteTooBigValueWithFalseFlagThrows) {
  const auto too_big_value = Varint::Limits::MAX_N4 + 1;
  ASSERT_THROW(
      Varint::writeUintWithFlagToBuffer(b4, sizeof b4, too_big_value, false),
      std::runtime_error);
}

TEST_F(VarintTestFixture, ReadMinN1Value) {
  Varint::writeUintToBuffer(b4, sizeof b4, Varint::Limits::MIN_N1);
  ASSERT_THAT(Varint::readUintFromBuffer(b4, sizeof b4, &value), Eq(1));
  ASSERT_THAT(value, Eq(Varint::Limits::MIN_N1));
}

TEST_F(VarintTestFixture, ReadMinN2Value) {
  Varint::writeUintToBuffer(b4, sizeof b4, Varint::Limits::MIN_N2);
  ASSERT_THAT(Varint::readUintFromBuffer(b4, sizeof b4, &value), Eq(2));
  ASSERT_THAT(value, Eq(Varint::Limits::MIN_N2));
}

TEST_F(VarintTestFixture, ReadMinN3Value) {
  Varint::writeUintToBuffer(b4, sizeof b4, Varint::Limits::MIN_N3);
  ASSERT_THAT(Varint::readUintFromBuffer(b4, sizeof b4, &value), Eq(3));
  ASSERT_THAT(value, Eq(Varint::Limits::MIN_N3));
}

TEST_F(VarintTestFixture, ReadMinN4Value) {
  Varint::writeUintToBuffer(b4, sizeof b4, Varint::Limits::MIN_N4);
  ASSERT_THAT(Varint::readUintFromBuffer(b4, sizeof b4, &value), Eq(4));
  ASSERT_THAT(value, Eq(Varint::Limits::MIN_N4));
}

TEST_F(VarintTestFixture, ReadMaxN1Value) {
  Varint::writeUintToBuffer(b4, sizeof b4, Varint::Limits::MAX_N1);
  ASSERT_THAT(Varint::readUintFromBuffer(b4, sizeof b4, &value), Eq(1));
  ASSERT_THAT(value, Eq(Varint::Limits::MAX_N1));
}

TEST_F(VarintTestFixture, ReadMaxN2Value) {
  Varint::writeUintToBuffer(b4, sizeof b4, Varint::Limits::MAX_N2);
  ASSERT_THAT(Varint::readUintFromBuffer(b4, sizeof b4, &value), Eq(2));
  ASSERT_THAT(value, Eq(Varint::Limits::MAX_N2));
}

TEST_F(VarintTestFixture, ReadMaxN3Value) {
  Varint::writeUintToBuffer(b4, sizeof b4, Varint::Limits::MAX_N3);
  ASSERT_THAT(Varint::readUintFromBuffer(b4, sizeof b4, &value), Eq(3));
  ASSERT_THAT(value, Eq(Varint::Limits::MAX_N3));
}

TEST_F(VarintTestFixture, ReadMaxN4Value) {
  Varint::writeUintToBuffer(b4, sizeof b4, Varint::Limits::MAX_N4);
  ASSERT_THAT(Varint::readUintFromBuffer(b4, sizeof b4, &value), Eq(4));
  ASSERT_THAT(value, Eq(Varint::Limits::MAX_N4));
}

TEST_F(VarintTestFixture, ReadValueFromNullPtrThrows) {
  ASSERT_THROW(Varint::readUintFromBuffer(nullptr, sizeof b4, &value),
               mt::AssertionError);
}

TEST_F(VarintTestFixture, ReadValueIntoNullPtrThrows) {
  ASSERT_THROW(Varint::readUintFromBuffer(b4, sizeof b4, nullptr),
               mt::AssertionError);
}

TEST_F(VarintTestFixture, ReadMinN1ValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlagToBuffer(b4, sizeof b4,
                                    Varint::Limits::MIN_N1_WITH_FLAG, true);
  ASSERT_THAT(Varint::readUintWithFlagFromBuffer(b4, sizeof b4, &value, &flag),
              Eq(1));
  ASSERT_THAT(value, Eq(Varint::Limits::MIN_N1_WITH_FLAG));
  ASSERT_THAT(flag, Eq(true));
}

TEST_F(VarintTestFixture, ReadMinN1ValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlagToBuffer(b4, sizeof b4,
                                    Varint::Limits::MIN_N1_WITH_FLAG, false);
  ASSERT_THAT(Varint::readUintWithFlagFromBuffer(b4, sizeof b4, &value, &flag),
              Eq(1));
  ASSERT_THAT(value, Eq(Varint::Limits::MIN_N1_WITH_FLAG));
  ASSERT_THAT(flag, Eq(false));
}

TEST_F(VarintTestFixture, ReadMinN2ValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlagToBuffer(b4, sizeof b4,
                                    Varint::Limits::MIN_N2_WITH_FLAG, true);
  ASSERT_THAT(Varint::readUintWithFlagFromBuffer(b4, sizeof b4, &value, &flag),
              Eq(2));
  ASSERT_THAT(value, Eq(Varint::Limits::MIN_N2_WITH_FLAG));
  ASSERT_THAT(flag, Eq(true));
}

TEST_F(VarintTestFixture, ReadMinN2ValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlagToBuffer(b4, sizeof b4,
                                    Varint::Limits::MIN_N2_WITH_FLAG, false);
  ASSERT_THAT(Varint::readUintWithFlagFromBuffer(b4, sizeof b4, &value, &flag),
              Eq(2));
  ASSERT_THAT(value, Eq(Varint::Limits::MIN_N2_WITH_FLAG));
  ASSERT_THAT(flag, Eq(false));
}

TEST_F(VarintTestFixture, ReadMinN3ValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlagToBuffer(b4, sizeof b4,
                                    Varint::Limits::MIN_N3_WITH_FLAG, true);
  ASSERT_THAT(Varint::readUintWithFlagFromBuffer(b4, sizeof b4, &value, &flag),
              Eq(3));
  ASSERT_THAT(value, Eq(Varint::Limits::MIN_N3_WITH_FLAG));
  ASSERT_THAT(flag, Eq(true));
}

TEST_F(VarintTestFixture, ReadMinN3ValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlagToBuffer(b4, sizeof b4,
                                    Varint::Limits::MIN_N3_WITH_FLAG, false);
  ASSERT_THAT(Varint::readUintWithFlagFromBuffer(b4, sizeof b4, &value, &flag),
              Eq(3));
  ASSERT_THAT(value, Eq(Varint::Limits::MIN_N3_WITH_FLAG));
  ASSERT_THAT(flag, Eq(false));
}

TEST_F(VarintTestFixture, ReadMinN4ValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlagToBuffer(b4, sizeof b4,
                                    Varint::Limits::MIN_N4_WITH_FLAG, true);
  ASSERT_THAT(Varint::readUintWithFlagFromBuffer(b4, sizeof b4, &value, &flag),
              Eq(4));
  ASSERT_THAT(value, Eq(Varint::Limits::MIN_N4_WITH_FLAG));
  ASSERT_THAT(flag, Eq(true));
}

TEST_F(VarintTestFixture, ReadMinN4ValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlagToBuffer(b4, sizeof b4,
                                    Varint::Limits::MIN_N4_WITH_FLAG, false);
  ASSERT_THAT(Varint::readUintWithFlagFromBuffer(b4, sizeof b4, &value, &flag),
              Eq(4));
  ASSERT_THAT(value, Eq(Varint::Limits::MIN_N4_WITH_FLAG));
  ASSERT_THAT(flag, Eq(false));
}

TEST_F(VarintTestFixture, ReadMaxN1ValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlagToBuffer(b4, sizeof b4,
                                    Varint::Limits::MAX_N1_WITH_FLAG, true);
  ASSERT_THAT(Varint::readUintWithFlagFromBuffer(b4, sizeof b4, &value, &flag),
              Eq(1));
  ASSERT_THAT(value, Eq(Varint::Limits::MAX_N1_WITH_FLAG));
  ASSERT_THAT(flag, Eq(true));
}

TEST_F(VarintTestFixture, ReadMaxN1ValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlagToBuffer(b4, sizeof b4,
                                    Varint::Limits::MAX_N1_WITH_FLAG, false);
  ASSERT_THAT(Varint::readUintWithFlagFromBuffer(b4, sizeof b4, &value, &flag),
              Eq(1));
  ASSERT_THAT(value, Eq(Varint::Limits::MAX_N1_WITH_FLAG));
  ASSERT_THAT(flag, Eq(false));
}

TEST_F(VarintTestFixture, ReadMaxN2ValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlagToBuffer(b4, sizeof b4,
                                    Varint::Limits::MAX_N2_WITH_FLAG, true);
  ASSERT_THAT(Varint::readUintWithFlagFromBuffer(b4, sizeof b4, &value, &flag),
              Eq(2));
  ASSERT_THAT(value, Eq(Varint::Limits::MAX_N2_WITH_FLAG));
  ASSERT_THAT(flag, Eq(true));
}

TEST_F(VarintTestFixture, ReadMaxN2ValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlagToBuffer(b4, sizeof b4,
                                    Varint::Limits::MAX_N2_WITH_FLAG, false);
  ASSERT_THAT(Varint::readUintWithFlagFromBuffer(b4, sizeof b4, &value, &flag),
              Eq(2));
  ASSERT_THAT(value, Eq(Varint::Limits::MAX_N2_WITH_FLAG));
  ASSERT_THAT(flag, Eq(false));
}

TEST_F(VarintTestFixture, ReadMaxN3ValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlagToBuffer(b4, sizeof b4,
                                    Varint::Limits::MAX_N3_WITH_FLAG, true);
  ASSERT_THAT(Varint::readUintWithFlagFromBuffer(b4, sizeof b4, &value, &flag),
              Eq(3));
  ASSERT_THAT(value, Eq(Varint::Limits::MAX_N3_WITH_FLAG));
  ASSERT_THAT(flag, Eq(true));
}

TEST_F(VarintTestFixture, ReadMaxN3ValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlagToBuffer(b4, sizeof b4,
                                    Varint::Limits::MAX_N3_WITH_FLAG, false);
  ASSERT_THAT(Varint::readUintWithFlagFromBuffer(b4, sizeof b4, &value, &flag),
              Eq(3));
  ASSERT_THAT(value, Eq(Varint::Limits::MAX_N3_WITH_FLAG));
  ASSERT_THAT(flag, Eq(false));
}

TEST_F(VarintTestFixture, ReadMaxN4ValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlagToBuffer(b4, sizeof b4,
                                    Varint::Limits::MAX_N4_WITH_FLAG, true);
  ASSERT_THAT(Varint::readUintWithFlagFromBuffer(b4, sizeof b4, &value, &flag),
              Eq(4));
  ASSERT_THAT(value, Eq(Varint::Limits::MAX_N4_WITH_FLAG));
  ASSERT_THAT(flag, Eq(true));
}

TEST_F(VarintTestFixture, ReadMaxN4ValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlagToBuffer(b4, sizeof b4,
                                    Varint::Limits::MAX_N4_WITH_FLAG, false);
  ASSERT_THAT(Varint::readUintWithFlagFromBuffer(b4, sizeof b4, &value, &flag),
              Eq(4));
  ASSERT_THAT(value, Eq(Varint::Limits::MAX_N4_WITH_FLAG));
  ASSERT_THAT(flag, Eq(false));
}

TEST_F(VarintTestFixture, ReadValueWithFlagFromNullPtrThrows) {
  ASSERT_THROW(
      Varint::readUintWithFlagFromBuffer(nullptr, sizeof b4, &value, &flag),
      mt::AssertionError);
}

TEST_F(VarintTestFixture, ReadValueWithFlagIntoNullValueThrows) {
  ASSERT_THROW(
      Varint::readUintWithFlagFromBuffer(b4, sizeof b4, nullptr, &flag),
      mt::AssertionError);
}

TEST_F(VarintTestFixture, ReadValueWithFlagIntoNullFlagThrows) {
  ASSERT_THROW(
      Varint::readUintWithFlagFromBuffer(b4, sizeof b4, &value, nullptr),
      mt::AssertionError);
}

TEST_F(VarintTestFixture, WriteAndReadSequenceOfValues) {
  uint32_t values[] = {(Varint::Limits::MAX_N1 - Varint::Limits::MIN_N1) / 2,
                       (Varint::Limits::MAX_N2 - Varint::Limits::MIN_N2) / 2,
                       (Varint::Limits::MAX_N3 - Varint::Limits::MIN_N3) / 2,
                       (Varint::Limits::MAX_N4 - Varint::Limits::MIN_N4) / 2};
  auto p = b32;
  p += Varint::writeUintToBuffer(p, sizeof b32 - (p - b32), values[0]);
  p += Varint::writeUintToBuffer(p, sizeof b32 - (p - b32), values[1]);
  p += Varint::writeUintToBuffer(p, sizeof b32 - (p - b32), values[2]);
  p += Varint::writeUintToBuffer(p, sizeof b32 - (p - b32), values[3]);
  ASSERT_THAT(p - b32, Eq(10));

  p = b32;
  p += Varint::readUintFromBuffer(p, sizeof b32 - (p - b32), &value);
  ASSERT_THAT(value, Eq(values[0]));
  p += Varint::readUintFromBuffer(p, sizeof b32 - (p - b32), &value);
  ASSERT_THAT(value, Eq(values[1]));
  p += Varint::readUintFromBuffer(p, sizeof b32 - (p - b32), &value);
  ASSERT_THAT(value, Eq(values[2]));
  p += Varint::readUintFromBuffer(p, sizeof b32 - (p - b32), &value);
  ASSERT_THAT(value, Eq(values[3]));
  ASSERT_THAT(p - b32, Eq(10));
}

TEST_F(VarintTestFixture, WriteAndReadSequenceOfValuesWithTrueFlags) {
  uint32_t values[] = {
      (Varint::Limits::MAX_N1_WITH_FLAG - Varint::Limits::MIN_N1_WITH_FLAG) / 2,
      (Varint::Limits::MAX_N2_WITH_FLAG - Varint::Limits::MIN_N2_WITH_FLAG) / 2,
      (Varint::Limits::MAX_N3_WITH_FLAG - Varint::Limits::MIN_N3_WITH_FLAG) / 2,
      (Varint::Limits::MAX_N4_WITH_FLAG - Varint::Limits::MIN_N4_WITH_FLAG) /
          2};
  auto p = b32;
  p += Varint::writeUintWithFlagToBuffer(p, sizeof b32 - (p - b32), values[0],
                                         true);
  p += Varint::writeUintWithFlagToBuffer(p, sizeof b32 - (p - b32), values[1],
                                         true);
  p += Varint::writeUintWithFlagToBuffer(p, sizeof b32 - (p - b32), values[2],
                                         true);
  p += Varint::writeUintWithFlagToBuffer(p, sizeof b32 - (p - b32), values[3],
                                         true);
  ASSERT_THAT(p - b32, Eq(10));

  p = b32;
  flag = false;
  p += Varint::readUintWithFlagFromBuffer(p, sizeof b32 - (p - b32), &value,
                                          &flag);
  ASSERT_THAT(value, Eq(values[0]));
  ASSERT_THAT(flag, Eq(true));

  flag = false;
  p += Varint::readUintWithFlagFromBuffer(p, sizeof b32 - (p - b32), &value,
                                          &flag);
  ASSERT_THAT(value, Eq(values[1]));
  ASSERT_THAT(flag, Eq(true));

  flag = false;
  p += Varint::readUintWithFlagFromBuffer(p, sizeof b32 - (p - b32), &value,
                                          &flag);
  ASSERT_THAT(value, Eq(values[2]));
  ASSERT_THAT(flag, Eq(true));

  flag = false;
  p += Varint::readUintWithFlagFromBuffer(p, sizeof b32 - (p - b32), &value,
                                          &flag);
  ASSERT_THAT(value, Eq(values[3]));
  ASSERT_THAT(flag, Eq(true));

  ASSERT_THAT(p - b32, Eq(10));
}

TEST_F(VarintTestFixture, WriteAndReadSequenceOfValuesWithFalseFlags) {
  uint32_t values[] = {
      (Varint::Limits::MAX_N1_WITH_FLAG - Varint::Limits::MIN_N1_WITH_FLAG) / 2,
      (Varint::Limits::MAX_N2_WITH_FLAG - Varint::Limits::MIN_N2_WITH_FLAG) / 2,
      (Varint::Limits::MAX_N3_WITH_FLAG - Varint::Limits::MIN_N3_WITH_FLAG) / 2,
      (Varint::Limits::MAX_N4_WITH_FLAG - Varint::Limits::MIN_N4_WITH_FLAG) /
          2};
  auto p = b32;
  p += Varint::writeUintWithFlagToBuffer(p, sizeof b32 - (p - b32), values[0],
                                         false);
  p += Varint::writeUintWithFlagToBuffer(p, sizeof b32 - (p - b32), values[1],
                                         false);
  p += Varint::writeUintWithFlagToBuffer(p, sizeof b32 - (p - b32), values[2],
                                         false);
  p += Varint::writeUintWithFlagToBuffer(p, sizeof b32 - (p - b32), values[3],
                                         false);
  ASSERT_THAT(p - b32, Eq(10));

  p = b32;
  flag = true;
  p += Varint::readUintWithFlagFromBuffer(p, sizeof b32 - (p - b32), &value,
                                          &flag);
  ASSERT_THAT(value, Eq(values[0]));
  ASSERT_THAT(flag, Eq(false));

  flag = true;
  p += Varint::readUintWithFlagFromBuffer(p, sizeof b32 - (p - b32), &value,
                                          &flag);
  ASSERT_THAT(value, Eq(values[1]));
  ASSERT_THAT(flag, Eq(false));

  flag = true;
  p += Varint::readUintWithFlagFromBuffer(p, sizeof b32 - (p - b32), &value,
                                          &flag);
  ASSERT_THAT(value, Eq(values[2]));
  ASSERT_THAT(flag, Eq(false));

  flag = true;
  p += Varint::readUintWithFlagFromBuffer(p, sizeof b32 - (p - b32), &value,
                                          &flag);
  ASSERT_THAT(value, Eq(values[3]));
  ASSERT_THAT(flag, Eq(false));

  ASSERT_THAT(p - b32, Eq(10));
}

TEST_F(VarintTestFixture, WriteAndReadSequenceOfValuesWithTrueAndFalseFlags) {
  uint32_t values[] = {
      (Varint::Limits::MAX_N1_WITH_FLAG - Varint::Limits::MIN_N1_WITH_FLAG) / 2,
      (Varint::Limits::MAX_N2_WITH_FLAG - Varint::Limits::MIN_N2_WITH_FLAG) / 2,
      (Varint::Limits::MAX_N3_WITH_FLAG - Varint::Limits::MIN_N3_WITH_FLAG) / 2,
      (Varint::Limits::MAX_N4_WITH_FLAG - Varint::Limits::MIN_N4_WITH_FLAG) /
          2};
  auto p = b32;
  p += Varint::writeUintWithFlagToBuffer(p, sizeof b32 - (p - b32), values[0],
                                         true);
  p += Varint::writeUintWithFlagToBuffer(p, sizeof b32 - (p - b32), values[1],
                                         false);
  p += Varint::writeUintWithFlagToBuffer(p, sizeof b32 - (p - b32), values[2],
                                         true);
  p += Varint::writeUintWithFlagToBuffer(p, sizeof b32 - (p - b32), values[3],
                                         false);
  ASSERT_THAT(p - b32, Eq(10));

  p = b32;
  flag = false;
  p += Varint::readUintWithFlagFromBuffer(p, sizeof b32 - (p - b32), &value,
                                          &flag);
  ASSERT_THAT(value, Eq(values[0]));
  ASSERT_THAT(flag, Eq(true));

  p += Varint::readUintWithFlagFromBuffer(p, sizeof b32 - (p - b32), &value,
                                          &flag);
  ASSERT_THAT(value, Eq(values[1]));
  ASSERT_THAT(flag, Eq(false));

  p += Varint::readUintWithFlagFromBuffer(p, sizeof b32 - (p - b32), &value,
                                          &flag);
  ASSERT_THAT(value, Eq(values[2]));
  ASSERT_THAT(flag, Eq(true));

  p += Varint::readUintWithFlagFromBuffer(p, sizeof b32 - (p - b32), &value,
                                          &flag);
  ASSERT_THAT(value, Eq(values[3]));
  ASSERT_THAT(flag, Eq(false));

  ASSERT_THAT(p - b32, Eq(10));
}

TEST_F(VarintTestFixture, WriteTrueFlag) {
  std::memset(b4, 0xDF, sizeof b4);
  ASSERT_NO_THROW(Varint::writeFlagToBuffer(b4, sizeof b4, true));
  ASSERT_THAT(b4, ElementsAre(0xFF, 0xDF, 0xDF, 0xDF));
}

TEST_F(VarintTestFixture, WriteTrueFlagToEmptyBufferThrows) {
  ASSERT_THROW(Varint::writeFlagToBuffer(b4, 0, true), mt::AssertionError);
}

TEST_F(VarintTestFixture, WriteFalseFlag) {
  std::memset(b4, 0xFF, sizeof b4);
  ASSERT_NO_THROW(Varint::writeFlagToBuffer(b4, sizeof b4, false));
  ASSERT_THAT(b4, ElementsAre(0xDF, 0xFF, 0xFF, 0xFF));
}

TEST_F(VarintTestFixture, WriteFalseFlagToEmptyBufferThrows) {
  ASSERT_THROW(Varint::writeFlagToBuffer(b4, 0, false), mt::AssertionError);
}

}  // namespace internal
}  // namespace multimap
