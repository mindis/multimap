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

#include <cstring>
#include <type_traits>
#include <gmock/gmock.h>
#include "multimap/internal/Varint.hpp"
#include "multimap/thirdparty/mt.hpp"

namespace multimap {
namespace internal {

struct VarintTestFixture : public testing::Test {
  void SetUp() override {
    std::memset(b32, 0, sizeof b32);
    std::memset(b4, 0, sizeof b4);
  }

  std::uint32_t value;
  bool flag;

  char b32[32];
  char b4[4];
};

TEST(VarintTest, IsNotDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<Varint>::value);
}

TEST_F(VarintTestFixture, WriteValueToNullPtrThrows) {
  ASSERT_THROW(Varint::writeUint(value, nullptr, 1), mt::AssertionError);
}

TEST_F(VarintTestFixture, WriteN1MinValueReturnsOne) {
  ASSERT_EQ(Varint::writeUint(Varint::Limits::N1_MIN_UINT, b4, sizeof b4), 1);
}

TEST_F(VarintTestFixture, WriteN1MaxValueReturnsOne) {
  ASSERT_EQ(Varint::writeUint(Varint::Limits::N1_MAX_UINT, b4, sizeof b4), 1);
}

TEST_F(VarintTestFixture, WriteN2MinValueReturnsTwo) {
  ASSERT_EQ(Varint::writeUint(Varint::Limits::N2_MIN_UINT, b4, sizeof b4), 2);
}

TEST_F(VarintTestFixture, WriteN2MaxValueReturnsTwo) {
  ASSERT_EQ(Varint::writeUint(Varint::Limits::N2_MAX_UINT, b4, sizeof b4), 2);
}

TEST_F(VarintTestFixture, WriteN3MinValueReturnsThree) {
  ASSERT_EQ(Varint::writeUint(Varint::Limits::N3_MIN_UINT, b4, sizeof b4), 3);
}

TEST_F(VarintTestFixture, WriteN3MaxValueReturnsThree) {
  ASSERT_EQ(Varint::writeUint(Varint::Limits::N3_MAX_UINT, b4, sizeof b4), 3);
}

TEST_F(VarintTestFixture, WriteN4MinValueReturnsFour) {
  ASSERT_EQ(Varint::writeUint(Varint::Limits::N4_MIN_UINT, b4, sizeof b4), 4);
}

TEST_F(VarintTestFixture, WriteN4MaxValueReturnsFour) {
  ASSERT_EQ(Varint::writeUint(Varint::Limits::N4_MAX_UINT, b4, sizeof b4), 4);
}

TEST_F(VarintTestFixture, WriteTooBigValueReturnsZero) {
  ASSERT_EQ(Varint::writeUint(Varint::Limits::N4_MAX_UINT + 1, b4, sizeof b4),
            0);
}

TEST_F(VarintTestFixture, WriteN1MinValueWithTrueFlagReturnsOne) {
  ASSERT_EQ(Varint::writeUintWithFlag(Varint::Limits::N1_MIN_UINT_WITH_FLAG,
                                      true, b4, sizeof b4),
            1);
}

TEST_F(VarintTestFixture, WriteN1MaxValueWithTrueFlagReturnsOne) {
  ASSERT_EQ(Varint::writeUintWithFlag(Varint::Limits::N1_MAX_UINT_WITH_FLAG,
                                      true, b4, sizeof b4),
            1);
}

TEST_F(VarintTestFixture, WriteN2MinValueWithTrueFlagReturnsTwo) {
  ASSERT_EQ(Varint::writeUintWithFlag(Varint::Limits::N2_MIN_UINT_WITH_FLAG,
                                      true, b4, sizeof b4),
            2);
}

TEST_F(VarintTestFixture, WriteN2MaxValueWithTrueFlagReturnsTwo) {
  ASSERT_EQ(Varint::writeUintWithFlag(Varint::Limits::N2_MAX_UINT_WITH_FLAG,
                                      true, b4, sizeof b4),
            2);
}

TEST_F(VarintTestFixture, WriteN3MinValueWithTrueFlagReturnsThree) {
  ASSERT_EQ(Varint::writeUintWithFlag(Varint::Limits::N3_MIN_UINT_WITH_FLAG,
                                      true, b4, sizeof b4),
            3);
}

TEST_F(VarintTestFixture, WriteN3MaxValueWithTrueFlagReturnsThree) {
  ASSERT_EQ(Varint::writeUintWithFlag(Varint::Limits::N3_MAX_UINT_WITH_FLAG,
                                      true, b4, sizeof b4),
            3);
}

TEST_F(VarintTestFixture, WriteN4MinValueWithTrueFlagReturnsFour) {
  ASSERT_EQ(Varint::writeUintWithFlag(Varint::Limits::N4_MIN_UINT_WITH_FLAG,
                                      true, b4, sizeof b4),
            4);
}

TEST_F(VarintTestFixture, WriteN4MaxValueWithTrueFlagReturnsFour) {
  ASSERT_EQ(Varint::writeUintWithFlag(Varint::Limits::N4_MAX_UINT_WITH_FLAG,
                                      true, b4, sizeof b4),
            4);
}

TEST_F(VarintTestFixture, WriteTooBigValueWithTrueFlagReturnsZero) {
  ASSERT_EQ(Varint::writeUintWithFlag(Varint::Limits::N4_MAX_UINT_WITH_FLAG + 1,
                                      true, b4, sizeof b4),
            0);
}

TEST_F(VarintTestFixture, WriteN1MinValueWithFalseFlagReturnsOne) {
  ASSERT_EQ(Varint::writeUintWithFlag(Varint::Limits::N1_MIN_UINT_WITH_FLAG,
                                      false, b4, sizeof b4),
            1);
}

TEST_F(VarintTestFixture, WriteN1MaxValueWithFalseFlagReturnsOne) {
  ASSERT_EQ(Varint::writeUintWithFlag(Varint::Limits::N1_MAX_UINT_WITH_FLAG,
                                      false, b4, sizeof b4),
            1);
}

TEST_F(VarintTestFixture, WriteN2MinValueWithFalseFlagReturnsTwo) {
  ASSERT_EQ(Varint::writeUintWithFlag(Varint::Limits::N2_MIN_UINT_WITH_FLAG,
                                      false, b4, sizeof b4),
            2);
}

TEST_F(VarintTestFixture, WriteN2MaxValueWithFalseFlagReturnsTwo) {
  ASSERT_EQ(Varint::writeUintWithFlag(Varint::Limits::N2_MAX_UINT_WITH_FLAG,
                                      false, b4, sizeof b4),
            2);
}

TEST_F(VarintTestFixture, WriteN3MinValueWithFalseFlagReturnsThree) {
  ASSERT_EQ(Varint::writeUintWithFlag(Varint::Limits::N3_MIN_UINT_WITH_FLAG,
                                      false, b4, sizeof b4),
            3);
}

TEST_F(VarintTestFixture, WriteN3MaxValueWithFalseFlagReturnsThree) {
  ASSERT_EQ(Varint::writeUintWithFlag(Varint::Limits::N3_MAX_UINT_WITH_FLAG,
                                      false, b4, sizeof b4),
            3);
}

TEST_F(VarintTestFixture, WriteN4MinValueWithFalseFlagReturnsFour) {
  ASSERT_EQ(Varint::writeUintWithFlag(Varint::Limits::N4_MIN_UINT_WITH_FLAG,
                                      false, b4, sizeof b4),
            4);
}

TEST_F(VarintTestFixture, WriteN4MaxValueWithFalseFlagReturnsFour) {
  ASSERT_EQ(Varint::writeUintWithFlag(Varint::Limits::N4_MAX_UINT_WITH_FLAG,
                                      false, b4, sizeof b4),
            4);
}

TEST_F(VarintTestFixture, WriteTooBigValueWithFalseFlagReturnsZero) {
  ASSERT_EQ(Varint::writeUintWithFlag(Varint::Limits::N4_MAX_UINT_WITH_FLAG + 1,
                                      false, b4, sizeof b4),
            0);
}

TEST_F(VarintTestFixture, ReadValueFromNullPtrThrows) {
  ASSERT_THROW(Varint::readUint(nullptr, sizeof b4, &value),
               mt::AssertionError);
}

TEST_F(VarintTestFixture, ReadValueIntoNullPtrThrows) {
  ASSERT_THROW(Varint::readUint(b4, sizeof b4, nullptr), mt::AssertionError);
}

TEST_F(VarintTestFixture, ReadN1MinValue) {
  Varint::writeUint(Varint::Limits::N1_MIN_UINT, b4, sizeof b4);
  ASSERT_EQ(Varint::readUint(b4, sizeof b4, &value), 1);
  ASSERT_EQ(value, Varint::Limits::N1_MIN_UINT);
}

TEST_F(VarintTestFixture, ReadN1MaxValue) {
  Varint::writeUint(Varint::Limits::N1_MAX_UINT, b4, sizeof b4);
  ASSERT_EQ(Varint::readUint(b4, sizeof b4, &value), 1);
  ASSERT_EQ(value, Varint::Limits::N1_MAX_UINT);
}

TEST_F(VarintTestFixture, ReadN2MinValue) {
  Varint::writeUint(Varint::Limits::N2_MIN_UINT, b4, sizeof b4);
  ASSERT_EQ(Varint::readUint(b4, sizeof b4, &value), 2);
  ASSERT_EQ(value, Varint::Limits::N2_MIN_UINT);
}

TEST_F(VarintTestFixture, ReadN2MaxValue) {
  Varint::writeUint(Varint::Limits::N2_MAX_UINT, b4, sizeof b4);
  ASSERT_EQ(Varint::readUint(b4, sizeof b4, &value), 2);
  ASSERT_EQ(value, Varint::Limits::N2_MAX_UINT);
}

TEST_F(VarintTestFixture, ReadN3MinValue) {
  Varint::writeUint(Varint::Limits::N3_MIN_UINT, b4, sizeof b4);
  ASSERT_EQ(Varint::readUint(b4, sizeof b4, &value), 3);
  ASSERT_EQ(value, Varint::Limits::N3_MIN_UINT);
}

TEST_F(VarintTestFixture, ReadN3MaxValue) {
  Varint::writeUint(Varint::Limits::N3_MAX_UINT, b4, sizeof b4);
  ASSERT_EQ(Varint::readUint(b4, sizeof b4, &value), 3);
  ASSERT_EQ(value, Varint::Limits::N3_MAX_UINT);
}

TEST_F(VarintTestFixture, ReadN4MinValue) {
  Varint::writeUint(Varint::Limits::N4_MIN_UINT, b4, sizeof b4);
  ASSERT_EQ(Varint::readUint(b4, sizeof b4, &value), 4);
  ASSERT_EQ(value, Varint::Limits::N4_MIN_UINT);
}

TEST_F(VarintTestFixture, ReadN4MaxValue) {
  Varint::writeUint(Varint::Limits::N4_MAX_UINT, b4, sizeof b4);
  ASSERT_EQ(Varint::readUint(b4, sizeof b4, &value), 4);
  ASSERT_EQ(value, Varint::Limits::N4_MAX_UINT);
}

TEST_F(VarintTestFixture, ReadValueWithFlagFromNullPtrThrows) {
  ASSERT_THROW(Varint::readUintWithFlag(nullptr, sizeof b4, &value, &flag),
               mt::AssertionError);
}

TEST_F(VarintTestFixture, ReadValueWithFlagIntoNullValueThrows) {
  ASSERT_THROW(Varint::readUintWithFlag(b4, sizeof b4, nullptr, &flag),
               mt::AssertionError);
}

TEST_F(VarintTestFixture, ReadValueWithFlagIntoNullFlagThrows) {
  ASSERT_THROW(Varint::readUintWithFlag(b4, sizeof b4, &value, nullptr),
               mt::AssertionError);
}

TEST_F(VarintTestFixture, ReadN1MinValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlag(Varint::Limits::N1_MIN_UINT_WITH_FLAG, true, b4,
                            sizeof b4);
  ASSERT_EQ(Varint::readUintWithFlag(b4, sizeof b4, &value, &flag), 1);
  ASSERT_EQ(value, Varint::Limits::N1_MIN_UINT_WITH_FLAG);
  ASSERT_EQ(flag, true);
}

TEST_F(VarintTestFixture, ReadN1MaxValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlag(Varint::Limits::N1_MAX_UINT_WITH_FLAG, true, b4,
                            sizeof b4);
  ASSERT_EQ(Varint::readUintWithFlag(b4, sizeof b4, &value, &flag), 1);
  ASSERT_EQ(value, Varint::Limits::N1_MAX_UINT_WITH_FLAG);
  ASSERT_EQ(flag, true);
}

TEST_F(VarintTestFixture, ReadN2MinValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlag(Varint::Limits::N2_MIN_UINT_WITH_FLAG, true, b4,
                            sizeof b4);
  ASSERT_EQ(Varint::readUintWithFlag(b4, sizeof b4, &value, &flag), 2);
  ASSERT_EQ(value, Varint::Limits::N2_MIN_UINT_WITH_FLAG);
  ASSERT_EQ(flag, true);
}

TEST_F(VarintTestFixture, ReadN2MaxValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlag(Varint::Limits::N2_MAX_UINT_WITH_FLAG, true, b4,
                            sizeof b4);
  ASSERT_EQ(Varint::readUintWithFlag(b4, sizeof b4, &value, &flag), 2);
  ASSERT_EQ(value, Varint::Limits::N2_MAX_UINT_WITH_FLAG);
  ASSERT_EQ(flag, true);
}

TEST_F(VarintTestFixture, ReadN3MinValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlag(Varint::Limits::N3_MIN_UINT_WITH_FLAG, true, b4,
                            sizeof b4);
  ASSERT_EQ(Varint::readUintWithFlag(b4, sizeof b4, &value, &flag), 3);
  ASSERT_EQ(value, Varint::Limits::N3_MIN_UINT_WITH_FLAG);
  ASSERT_EQ(flag, true);
}

TEST_F(VarintTestFixture, ReadN3MaxValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlag(Varint::Limits::N3_MAX_UINT_WITH_FLAG, true, b4,
                            sizeof b4);
  ASSERT_EQ(Varint::readUintWithFlag(b4, sizeof b4, &value, &flag), 3);
  ASSERT_EQ(value, Varint::Limits::N3_MAX_UINT_WITH_FLAG);
  ASSERT_EQ(flag, true);
}

TEST_F(VarintTestFixture, ReadN4MinValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlag(Varint::Limits::N4_MIN_UINT_WITH_FLAG, true, b4,
                            sizeof b4);
  ASSERT_EQ(Varint::readUintWithFlag(b4, sizeof b4, &value, &flag), 4);
  ASSERT_EQ(value, Varint::Limits::N4_MIN_UINT_WITH_FLAG);
  ASSERT_EQ(flag, true);
}

TEST_F(VarintTestFixture, ReadN4MaxValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlag(Varint::Limits::N4_MAX_UINT_WITH_FLAG, true, b4,
                            sizeof b4);
  ASSERT_EQ(Varint::readUintWithFlag(b4, sizeof b4, &value, &flag), 4);
  ASSERT_EQ(value, Varint::Limits::N4_MAX_UINT_WITH_FLAG);
  ASSERT_EQ(flag, true);
}

TEST_F(VarintTestFixture, ReadN1MinValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlag(Varint::Limits::N1_MIN_UINT_WITH_FLAG, false, b4,
                            sizeof b4);
  ASSERT_EQ(Varint::readUintWithFlag(b4, sizeof b4, &value, &flag), 1);
  ASSERT_EQ(value, Varint::Limits::N1_MIN_UINT_WITH_FLAG);
  ASSERT_EQ(flag, false);
}

TEST_F(VarintTestFixture, ReadN1MaxValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlag(Varint::Limits::N1_MAX_UINT_WITH_FLAG, false, b4,
                            sizeof b4);
  ASSERT_EQ(Varint::readUintWithFlag(b4, sizeof b4, &value, &flag), 1);
  ASSERT_EQ(value, Varint::Limits::N1_MAX_UINT_WITH_FLAG);
  ASSERT_EQ(flag, false);
}

TEST_F(VarintTestFixture, ReadN2MinValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlag(Varint::Limits::N2_MIN_UINT_WITH_FLAG, false, b4,
                            sizeof b4);
  ASSERT_EQ(Varint::readUintWithFlag(b4, sizeof b4, &value, &flag), 2);
  ASSERT_EQ(value, Varint::Limits::N2_MIN_UINT_WITH_FLAG);
  ASSERT_EQ(flag, false);
}

TEST_F(VarintTestFixture, ReadN2MaxValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlag(Varint::Limits::N2_MAX_UINT_WITH_FLAG, false, b4,
                            sizeof b4);
  ASSERT_EQ(Varint::readUintWithFlag(b4, sizeof b4, &value, &flag), 2);
  ASSERT_EQ(value, Varint::Limits::N2_MAX_UINT_WITH_FLAG);
  ASSERT_EQ(flag, false);
}

TEST_F(VarintTestFixture, ReadN3MinValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlag(Varint::Limits::N3_MIN_UINT_WITH_FLAG, false, b4,
                            sizeof b4);
  ASSERT_EQ(Varint::readUintWithFlag(b4, sizeof b4, &value, &flag), 3);
  ASSERT_EQ(value, Varint::Limits::N3_MIN_UINT_WITH_FLAG);
  ASSERT_EQ(flag, false);
}

TEST_F(VarintTestFixture, ReadN3MaxValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlag(Varint::Limits::N3_MAX_UINT_WITH_FLAG, false, b4,
                            sizeof b4);
  ASSERT_EQ(Varint::readUintWithFlag(b4, sizeof b4, &value, &flag), 3);
  ASSERT_EQ(value, Varint::Limits::N3_MAX_UINT_WITH_FLAG);
  ASSERT_EQ(flag, false);
}

TEST_F(VarintTestFixture, ReadN4MinValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlag(Varint::Limits::N4_MIN_UINT_WITH_FLAG, false, b4,
                            sizeof b4);
  ASSERT_EQ(Varint::readUintWithFlag(b4, sizeof b4, &value, &flag), 4);
  ASSERT_EQ(value, Varint::Limits::N4_MIN_UINT_WITH_FLAG);
  ASSERT_EQ(flag, false);
}

TEST_F(VarintTestFixture, ReadN4MaxValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlag(Varint::Limits::N4_MAX_UINT_WITH_FLAG, false, b4,
                            sizeof b4);
  ASSERT_EQ(Varint::readUintWithFlag(b4, sizeof b4, &value, &flag), 4);
  ASSERT_EQ(value, Varint::Limits::N4_MAX_UINT_WITH_FLAG);
  ASSERT_EQ(flag, false);
}

TEST_F(VarintTestFixture, WriteAndReadSomeValues) {
  std::uint32_t values[] = {
    (Varint::Limits::N1_MAX_UINT - Varint::Limits::N1_MIN_UINT) / 2,
    (Varint::Limits::N2_MAX_UINT - Varint::Limits::N2_MIN_UINT) / 2,
    (Varint::Limits::N3_MAX_UINT - Varint::Limits::N3_MIN_UINT) / 2,
    (Varint::Limits::N4_MAX_UINT - Varint::Limits::N4_MIN_UINT) / 2
  };
  auto p = b32;
  p += Varint::writeUint(values[0], p, sizeof b32 - (p - b32));
  p += Varint::writeUint(values[1], p, sizeof b32 - (p - b32));
  p += Varint::writeUint(values[2], p, sizeof b32 - (p - b32));
  p += Varint::writeUint(values[3], p, sizeof b32 - (p - b32));
  ASSERT_EQ(p - b32, 10);

  p = b32;
  p += Varint::readUint(p, sizeof b32 - (p - b32), &value);
  ASSERT_EQ(value, values[0]);
  p += Varint::readUint(p, sizeof b32 - (p - b32), &value);
  ASSERT_EQ(value, values[1]);
  p += Varint::readUint(p, sizeof b32 - (p - b32), &value);
  ASSERT_EQ(value, values[2]);
  p += Varint::readUint(p, sizeof b32 - (p - b32), &value);
  ASSERT_EQ(value, values[3]);
  ASSERT_EQ(p - b32, 10);
}

TEST_F(VarintTestFixture, WriteAndReadSomeValuesWithTrueFlags) {
  std::uint32_t values[] = {(Varint::Limits::N1_MAX_UINT_WITH_FLAG -
                             Varint::Limits::N1_MIN_UINT_WITH_FLAG) /
                                2,
                            (Varint::Limits::N2_MAX_UINT_WITH_FLAG -
                             Varint::Limits::N2_MIN_UINT_WITH_FLAG) /
                                2,
                            (Varint::Limits::N3_MAX_UINT_WITH_FLAG -
                             Varint::Limits::N3_MIN_UINT_WITH_FLAG) /
                                2,
                            (Varint::Limits::N4_MAX_UINT_WITH_FLAG -
                             Varint::Limits::N4_MIN_UINT_WITH_FLAG) /
                                2 };
  auto p = b32;
  p += Varint::writeUintWithFlag(values[0], true, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[1], true, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[2], true, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[3], true, p, sizeof b32 - (p - b32));
  ASSERT_EQ(p - b32, 10);

  p = b32;
  flag = false;
  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_EQ(value, values[0]);
  ASSERT_EQ(flag, true);

  flag = false;
  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_EQ(value, values[1]);
  ASSERT_EQ(flag, true);

  flag = false;
  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_EQ(value, values[2]);
  ASSERT_EQ(flag, true);

  flag = false;
  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_EQ(value, values[3]);
  ASSERT_EQ(flag, true);

  ASSERT_EQ(p - b32, 10);
}

TEST_F(VarintTestFixture, WriteAndReadSomeValuesWithFalseFlags) {
  std::uint32_t values[] = {(Varint::Limits::N1_MAX_UINT_WITH_FLAG -
                             Varint::Limits::N1_MIN_UINT_WITH_FLAG) /
                                2,
                            (Varint::Limits::N2_MAX_UINT_WITH_FLAG -
                             Varint::Limits::N2_MIN_UINT_WITH_FLAG) /
                                2,
                            (Varint::Limits::N3_MAX_UINT_WITH_FLAG -
                             Varint::Limits::N3_MIN_UINT_WITH_FLAG) /
                                2,
                            (Varint::Limits::N4_MAX_UINT_WITH_FLAG -
                             Varint::Limits::N4_MIN_UINT_WITH_FLAG) /
                                2 };
  auto p = b32;
  p += Varint::writeUintWithFlag(values[0], false, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[1], false, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[2], false, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[3], false, p, sizeof b32 - (p - b32));
  ASSERT_EQ(p - b32, 10);

  p = b32;
  flag = true;
  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_EQ(value, values[0]);
  ASSERT_EQ(flag, false);

  flag = true;
  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_EQ(value, values[1]);
  ASSERT_EQ(flag, false);

  flag = true;
  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_EQ(value, values[2]);
  ASSERT_EQ(flag, false);

  flag = true;
  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_EQ(value, values[3]);
  ASSERT_EQ(flag, false);

  ASSERT_EQ(p - b32, 10);
}

TEST_F(VarintTestFixture, WriteAndReadSomeValuesWithTrueAndFalseFlags) {
  std::uint32_t values[] = {(Varint::Limits::N1_MAX_UINT_WITH_FLAG -
                             Varint::Limits::N1_MIN_UINT_WITH_FLAG) /
                                2,
                            (Varint::Limits::N2_MAX_UINT_WITH_FLAG -
                             Varint::Limits::N2_MIN_UINT_WITH_FLAG) /
                                2,
                            (Varint::Limits::N3_MAX_UINT_WITH_FLAG -
                             Varint::Limits::N3_MIN_UINT_WITH_FLAG) /
                                2,
                            (Varint::Limits::N4_MAX_UINT_WITH_FLAG -
                             Varint::Limits::N4_MIN_UINT_WITH_FLAG) /
                                2 };
  auto p = b32;
  p += Varint::writeUintWithFlag(values[0], true, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[1], false, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[2], true, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[3], false, p, sizeof b32 - (p - b32));
  ASSERT_EQ(p - b32, 10);

  p = b32;
  flag = false;
  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_EQ(value, values[0]);
  ASSERT_EQ(flag, true);

  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_EQ(value, values[1]);
  ASSERT_EQ(flag, false);

  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_EQ(value, values[2]);
  ASSERT_EQ(flag, true);

  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_EQ(value, values[3]);
  ASSERT_EQ(flag, false);

  ASSERT_EQ(p - b32, 10);
}

} // namespace internal
} // namespace multimap
