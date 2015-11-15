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

using testing::ElementsAre;

struct VarintTestFixture : public testing::Test {
  void SetUp() override {
    std::memset(b32, 0, sizeof b32);
    std::memset(b5, 0, sizeof b5);
  }

  std::uint32_t value;
  bool flag;

  char b32[32];
  char b5[5];
};

TEST(VarintTest, IsNotDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<Varint>::value);
}

TEST_F(VarintTestFixture, WriteValueToNullPtrThrows) {
  ASSERT_THROW(Varint::writeUint(value, nullptr, 1), mt::AssertionError);
}

TEST_F(VarintTestFixture, WriteN1MinValue) {
  ASSERT_THAT(Varint::writeUint(Varint::Limits::N1_MIN_UINT, b5, sizeof b5), 1);
  ASSERT_THAT(b5, ElementsAre(0x00, 0x00, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteN1MaxValue) {
  ASSERT_THAT(Varint::writeUint(Varint::Limits::N1_MAX_UINT, b5, sizeof b5), 1);
  ASSERT_THAT(b5, ElementsAre(0x7F, 0x00, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteN2MinValue) {
  ASSERT_THAT(Varint::writeUint(Varint::Limits::N2_MIN_UINT, b5, sizeof b5), 2);
  ASSERT_THAT(b5, ElementsAre(0x80, 0x01, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteN2MaxValue) {
  ASSERT_THAT(Varint::writeUint(Varint::Limits::N2_MAX_UINT, b5, sizeof b5), 2);
  ASSERT_THAT(b5, ElementsAre(0xFF, 0x7F, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteN3MinValue) {
  ASSERT_THAT(Varint::writeUint(Varint::Limits::N3_MIN_UINT, b5, sizeof b5), 3);
  ASSERT_THAT(b5, ElementsAre(0x80, 0x80, 0x01, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteN3MaxValue) {
  ASSERT_THAT(Varint::writeUint(Varint::Limits::N3_MAX_UINT, b5, sizeof b5), 3);
  ASSERT_THAT(b5, ElementsAre(0xFF, 0xFF, 0x7F, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteN4MinValue) {
  ASSERT_THAT(Varint::writeUint(Varint::Limits::N4_MIN_UINT, b5, sizeof b5), 4);
  ASSERT_THAT(b5, ElementsAre(0x80, 0x80, 0x80, 0x01, 0x00));
}

TEST_F(VarintTestFixture, WriteN4MaxValue) {
  ASSERT_THAT(Varint::writeUint(Varint::Limits::N4_MAX_UINT, b5, sizeof b5), 4);
  ASSERT_THAT(b5, ElementsAre(0xFF, 0xFF, 0xFF, 0x7F, 0x00));
}

TEST_F(VarintTestFixture, WriteN5MinValue) {
  ASSERT_THAT(Varint::writeUint(Varint::Limits::N5_MIN_UINT, b5, sizeof b5), 5);
  ASSERT_THAT(b5, ElementsAre(0x80, 0x80, 0x80, 0x80, 0x01));
}

TEST_F(VarintTestFixture, WriteN5MaxValue) {
  ASSERT_THAT(Varint::writeUint(Varint::Limits::N5_MAX_UINT, b5, sizeof b5), 5);
  ASSERT_THAT(b5, ElementsAre(0xFF, 0xFF, 0xFF, 0xFF, 0x0F));
}

TEST_F(VarintTestFixture, WriteN1MinValueWithTrueFlag) {
  ASSERT_THAT(Varint::writeUintWithFlag(Varint::Limits::N1_MIN_UINT_WITH_FLAG,
                                        true, b5, sizeof b5),
              1);
  ASSERT_THAT(b5, ElementsAre(0x40, 0x00, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteN1MinValueWithFalseFlag) {
  ASSERT_THAT(Varint::writeUintWithFlag(Varint::Limits::N1_MIN_UINT_WITH_FLAG,
                                        false, b5, sizeof b5),
              1);
  ASSERT_THAT(b5, ElementsAre(0x00, 0x00, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteN1MaxValueWithTrueFlag) {
  ASSERT_THAT(Varint::writeUintWithFlag(Varint::Limits::N1_MAX_UINT_WITH_FLAG,
                                        true, b5, sizeof b5),
              1);
  ASSERT_THAT(b5, ElementsAre(0x7F, 0x00, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteN1MaxValueWithFalseFlag) {
  ASSERT_THAT(Varint::writeUintWithFlag(Varint::Limits::N1_MAX_UINT_WITH_FLAG,
                                        false, b5, sizeof b5),
              1);
  ASSERT_THAT(b5, ElementsAre(0x3F, 0x00, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteN2MinValueWithTrueFlag) {
  ASSERT_THAT(Varint::writeUintWithFlag(Varint::Limits::N2_MIN_UINT_WITH_FLAG,
                                        true, b5, sizeof b5),
              2);
  ASSERT_THAT(b5, ElementsAre(0xC0, 0x01, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteN2MinValueWithFalseFlag) {
  ASSERT_THAT(Varint::writeUintWithFlag(Varint::Limits::N2_MIN_UINT_WITH_FLAG,
                                        false, b5, sizeof b5),
              2);
  ASSERT_THAT(b5, ElementsAre(0x80, 0x01, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteN2MaxValueWithTrueFlag) {
  ASSERT_THAT(Varint::writeUintWithFlag(Varint::Limits::N2_MAX_UINT_WITH_FLAG,
                                        true, b5, sizeof b5),
              2);
  ASSERT_THAT(b5, ElementsAre(0xFF, 0x7F, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteN2MaxValueWithFalseFlag) {
  ASSERT_THAT(Varint::writeUintWithFlag(Varint::Limits::N2_MAX_UINT_WITH_FLAG,
                                        false, b5, sizeof b5),
              2);
  ASSERT_THAT(b5, ElementsAre(0xBF, 0x7F, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteN3MinValueWithTrueFlag) {
  ASSERT_THAT(Varint::writeUintWithFlag(Varint::Limits::N3_MIN_UINT_WITH_FLAG,
                                        true, b5, sizeof b5),
              3);
  ASSERT_THAT(b5, ElementsAre(0xC0, 0x80, 0x01, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteN3MinValueWithFalseFlag) {
  ASSERT_THAT(Varint::writeUintWithFlag(Varint::Limits::N3_MIN_UINT_WITH_FLAG,
                                        false, b5, sizeof b5),
              3);
  ASSERT_THAT(b5, ElementsAre(0x80, 0x80, 0x01, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteN3MaxValueWithTrueFlag) {
  ASSERT_THAT(Varint::writeUintWithFlag(Varint::Limits::N3_MAX_UINT_WITH_FLAG,
                                        true, b5, sizeof b5),
              3);
  ASSERT_THAT(b5, ElementsAre(0xFF, 0xFF, 0x7F, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteN3MaxValueWithFalseFlag) {
  ASSERT_THAT(Varint::writeUintWithFlag(Varint::Limits::N3_MAX_UINT_WITH_FLAG,
                                        false, b5, sizeof b5),
              3);
  ASSERT_THAT(b5, ElementsAre(0xBF, 0xFF, 0x7F, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteN4MinValueWithTrueFlag) {
  ASSERT_THAT(Varint::writeUintWithFlag(Varint::Limits::N4_MIN_UINT_WITH_FLAG,
                                        true, b5, sizeof b5),
              4);
  ASSERT_THAT(b5, ElementsAre(0xC0, 0x80, 0x80, 0x01, 0x00));
}

TEST_F(VarintTestFixture, WriteN4MinValueWithFalseFlag) {
  ASSERT_THAT(Varint::writeUintWithFlag(Varint::Limits::N4_MIN_UINT_WITH_FLAG,
                                        false, b5, sizeof b5),
              4);
  ASSERT_THAT(b5, ElementsAre(0x80, 0x80, 0x80, 0x01, 0x00));
}

TEST_F(VarintTestFixture, WriteN4MaxValueWithTrueFlag) {
  ASSERT_THAT(Varint::writeUintWithFlag(Varint::Limits::N4_MAX_UINT_WITH_FLAG,
                                        true, b5, sizeof b5),
              4);
  ASSERT_THAT(b5, ElementsAre(0xFF, 0xFF, 0xFF, 0x7F, 0x00));
}

TEST_F(VarintTestFixture, WriteN4MaxValueWithFalseFlag) {
  ASSERT_THAT(Varint::writeUintWithFlag(Varint::Limits::N4_MAX_UINT_WITH_FLAG,
                                        false, b5, sizeof b5),
              4);
  ASSERT_THAT(b5, ElementsAre(0xBF, 0xFF, 0xFF, 0x7F, 0x00));
}

TEST_F(VarintTestFixture, WriteN5MinValueWithTrueFlag) {
  ASSERT_THAT(Varint::writeUintWithFlag(Varint::Limits::N5_MIN_UINT_WITH_FLAG,
                                        true, b5, sizeof b5),
              5);
  ASSERT_THAT(b5, ElementsAre(0xC0, 0x80, 0x80, 0x80, 0x01));
}

TEST_F(VarintTestFixture, WriteN5MinValueWithFalseFlag) {
  ASSERT_THAT(Varint::writeUintWithFlag(Varint::Limits::N5_MIN_UINT_WITH_FLAG,
                                        false, b5, sizeof b5),
              5);
  ASSERT_THAT(b5, ElementsAre(0x80, 0x80, 0x80, 0x80, 0x01));
}

TEST_F(VarintTestFixture, WriteN5MaxValueWithTrueFlag) {
  ASSERT_THAT(Varint::writeUintWithFlag(Varint::Limits::N5_MAX_UINT_WITH_FLAG,
                                        true, b5, sizeof b5),
              5);
  ASSERT_THAT(b5, ElementsAre(0xFF, 0xFF, 0xFF, 0xFF, 0x1F));
}

TEST_F(VarintTestFixture, WriteN5MaxValueWithFalseFlag) {
  ASSERT_THAT(Varint::writeUintWithFlag(Varint::Limits::N5_MAX_UINT_WITH_FLAG,
                                        false, b5, sizeof b5),
              5);
  ASSERT_THAT(b5, ElementsAre(0xBF, 0xFF, 0xFF, 0xFF, 0x1F));
}

TEST_F(VarintTestFixture, ReadValueFromNullPtrThrows) {
  ASSERT_THROW(Varint::readUint(nullptr, sizeof b5, &value),
               mt::AssertionError);
}

TEST_F(VarintTestFixture, ReadValueIntoNullPtrThrows) {
  ASSERT_THROW(Varint::readUint(b5, sizeof b5, nullptr), mt::AssertionError);
}

TEST_F(VarintTestFixture, ReadN1MinValue) {
  Varint::writeUint(Varint::Limits::N1_MIN_UINT, b5, sizeof b5);
  ASSERT_THAT(Varint::readUint(b5, sizeof b5, &value), 1);
  ASSERT_THAT(value, Varint::Limits::N1_MIN_UINT);
}

TEST_F(VarintTestFixture, ReadN1MaxValue) {
  Varint::writeUint(Varint::Limits::N1_MAX_UINT, b5, sizeof b5);
  ASSERT_THAT(Varint::readUint(b5, sizeof b5, &value), 1);
  ASSERT_THAT(value, Varint::Limits::N1_MAX_UINT);
}

TEST_F(VarintTestFixture, ReadN2MinValue) {
  Varint::writeUint(Varint::Limits::N2_MIN_UINT, b5, sizeof b5);
  ASSERT_THAT(Varint::readUint(b5, sizeof b5, &value), 2);
  ASSERT_THAT(value, Varint::Limits::N2_MIN_UINT);
}

TEST_F(VarintTestFixture, ReadN2MaxValue) {
  Varint::writeUint(Varint::Limits::N2_MAX_UINT, b5, sizeof b5);
  ASSERT_THAT(Varint::readUint(b5, sizeof b5, &value), 2);
  ASSERT_THAT(value, Varint::Limits::N2_MAX_UINT);
}

TEST_F(VarintTestFixture, ReadN3MinValue) {
  Varint::writeUint(Varint::Limits::N3_MIN_UINT, b5, sizeof b5);
  ASSERT_THAT(Varint::readUint(b5, sizeof b5, &value), 3);
  ASSERT_THAT(value, Varint::Limits::N3_MIN_UINT);
}

TEST_F(VarintTestFixture, ReadN3MaxValue) {
  Varint::writeUint(Varint::Limits::N3_MAX_UINT, b5, sizeof b5);
  ASSERT_THAT(Varint::readUint(b5, sizeof b5, &value), 3);
  ASSERT_THAT(value, Varint::Limits::N3_MAX_UINT);
}

TEST_F(VarintTestFixture, ReadN4MinValue) {
  Varint::writeUint(Varint::Limits::N4_MIN_UINT, b5, sizeof b5);
  ASSERT_THAT(Varint::readUint(b5, sizeof b5, &value), 4);
  ASSERT_THAT(value, Varint::Limits::N4_MIN_UINT);
}

TEST_F(VarintTestFixture, ReadN4MaxValue) {
  Varint::writeUint(Varint::Limits::N4_MAX_UINT, b5, sizeof b5);
  ASSERT_THAT(Varint::readUint(b5, sizeof b5, &value), 4);
  ASSERT_THAT(value, Varint::Limits::N4_MAX_UINT);
}

TEST_F(VarintTestFixture, ReadN5MinValue) {
  Varint::writeUint(Varint::Limits::N5_MIN_UINT, b5, sizeof b5);
  ASSERT_THAT(Varint::readUint(b5, sizeof b5, &value), 5);
  ASSERT_THAT(value, Varint::Limits::N5_MIN_UINT);
}

TEST_F(VarintTestFixture, ReadN5MaxValue) {
  Varint::writeUint(Varint::Limits::N5_MAX_UINT, b5, sizeof b5);
  ASSERT_THAT(Varint::readUint(b5, sizeof b5, &value), 5);
  ASSERT_THAT(value, Varint::Limits::N5_MAX_UINT);
}

TEST_F(VarintTestFixture, ReadValueWithFlagFromNullPtrThrows) {
  ASSERT_THROW(Varint::readUintWithFlag(nullptr, sizeof b5, &value, &flag),
               mt::AssertionError);
}

TEST_F(VarintTestFixture, ReadValueWithFlagIntoNullValueThrows) {
  ASSERT_THROW(Varint::readUintWithFlag(b5, sizeof b5, nullptr, &flag),
               mt::AssertionError);
}

TEST_F(VarintTestFixture, ReadValueWithFlagIntoNullFlagThrows) {
  ASSERT_THROW(Varint::readUintWithFlag(b5, sizeof b5, &value, nullptr),
               mt::AssertionError);
}

TEST_F(VarintTestFixture, ReadN1MinValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlag(Varint::Limits::N1_MIN_UINT_WITH_FLAG, true, b5,
                            sizeof b5);
  ASSERT_THAT(Varint::readUintWithFlag(b5, sizeof b5, &value, &flag), 1);
  ASSERT_THAT(value, Varint::Limits::N1_MIN_UINT_WITH_FLAG);
  ASSERT_THAT(flag, true);
}

TEST_F(VarintTestFixture, ReadN1MinValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlag(Varint::Limits::N1_MIN_UINT_WITH_FLAG, false, b5,
                            sizeof b5);
  ASSERT_THAT(Varint::readUintWithFlag(b5, sizeof b5, &value, &flag), 1);
  ASSERT_THAT(value, Varint::Limits::N1_MIN_UINT_WITH_FLAG);
  ASSERT_THAT(flag, false);
}

TEST_F(VarintTestFixture, ReadN1MaxValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlag(Varint::Limits::N1_MAX_UINT_WITH_FLAG, true, b5,
                            sizeof b5);
  ASSERT_THAT(Varint::readUintWithFlag(b5, sizeof b5, &value, &flag), 1);
  ASSERT_THAT(value, Varint::Limits::N1_MAX_UINT_WITH_FLAG);
  ASSERT_THAT(flag, true);
}

TEST_F(VarintTestFixture, ReadN1MaxValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlag(Varint::Limits::N1_MAX_UINT_WITH_FLAG, false, b5,
                            sizeof b5);
  ASSERT_THAT(Varint::readUintWithFlag(b5, sizeof b5, &value, &flag), 1);
  ASSERT_THAT(value, Varint::Limits::N1_MAX_UINT_WITH_FLAG);
  ASSERT_THAT(flag, false);
}

TEST_F(VarintTestFixture, ReadN2MinValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlag(Varint::Limits::N2_MIN_UINT_WITH_FLAG, true, b5,
                            sizeof b5);
  ASSERT_THAT(Varint::readUintWithFlag(b5, sizeof b5, &value, &flag), 2);
  ASSERT_THAT(value, Varint::Limits::N2_MIN_UINT_WITH_FLAG);
  ASSERT_THAT(flag, true);
}

TEST_F(VarintTestFixture, ReadN2MinValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlag(Varint::Limits::N2_MIN_UINT_WITH_FLAG, false, b5,
                            sizeof b5);
  ASSERT_THAT(Varint::readUintWithFlag(b5, sizeof b5, &value, &flag), 2);
  ASSERT_THAT(value, Varint::Limits::N2_MIN_UINT_WITH_FLAG);
  ASSERT_THAT(flag, false);
}
TEST_F(VarintTestFixture, ReadN2MaxValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlag(Varint::Limits::N2_MAX_UINT_WITH_FLAG, true, b5,
                            sizeof b5);
  ASSERT_THAT(Varint::readUintWithFlag(b5, sizeof b5, &value, &flag), 2);
  ASSERT_THAT(value, Varint::Limits::N2_MAX_UINT_WITH_FLAG);
  ASSERT_THAT(flag, true);
}

TEST_F(VarintTestFixture, ReadN2MaxValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlag(Varint::Limits::N2_MAX_UINT_WITH_FLAG, false, b5,
                            sizeof b5);
  ASSERT_THAT(Varint::readUintWithFlag(b5, sizeof b5, &value, &flag), 2);
  ASSERT_THAT(value, Varint::Limits::N2_MAX_UINT_WITH_FLAG);
  ASSERT_THAT(flag, false);
}

TEST_F(VarintTestFixture, ReadN3MinValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlag(Varint::Limits::N3_MIN_UINT_WITH_FLAG, true, b5,
                            sizeof b5);
  ASSERT_THAT(Varint::readUintWithFlag(b5, sizeof b5, &value, &flag), 3);
  ASSERT_THAT(value, Varint::Limits::N3_MIN_UINT_WITH_FLAG);
  ASSERT_THAT(flag, true);
}

TEST_F(VarintTestFixture, ReadN3MinValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlag(Varint::Limits::N3_MIN_UINT_WITH_FLAG, false, b5,
                            sizeof b5);
  ASSERT_THAT(Varint::readUintWithFlag(b5, sizeof b5, &value, &flag), 3);
  ASSERT_THAT(value, Varint::Limits::N3_MIN_UINT_WITH_FLAG);
  ASSERT_THAT(flag, false);
}

TEST_F(VarintTestFixture, ReadN3MaxValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlag(Varint::Limits::N3_MAX_UINT_WITH_FLAG, true, b5,
                            sizeof b5);
  ASSERT_THAT(Varint::readUintWithFlag(b5, sizeof b5, &value, &flag), 3);
  ASSERT_THAT(value, Varint::Limits::N3_MAX_UINT_WITH_FLAG);
  ASSERT_THAT(flag, true);
}

TEST_F(VarintTestFixture, ReadN3MaxValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlag(Varint::Limits::N3_MAX_UINT_WITH_FLAG, false, b5,
                            sizeof b5);
  ASSERT_THAT(Varint::readUintWithFlag(b5, sizeof b5, &value, &flag), 3);
  ASSERT_THAT(value, Varint::Limits::N3_MAX_UINT_WITH_FLAG);
  ASSERT_THAT(flag, false);
}

TEST_F(VarintTestFixture, ReadN4MinValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlag(Varint::Limits::N4_MIN_UINT_WITH_FLAG, true, b5,
                            sizeof b5);
  ASSERT_THAT(Varint::readUintWithFlag(b5, sizeof b5, &value, &flag), 4);
  ASSERT_THAT(value, Varint::Limits::N4_MIN_UINT_WITH_FLAG);
  ASSERT_THAT(flag, true);
}

TEST_F(VarintTestFixture, ReadN4MinValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlag(Varint::Limits::N4_MIN_UINT_WITH_FLAG, false, b5,
                            sizeof b5);
  ASSERT_THAT(Varint::readUintWithFlag(b5, sizeof b5, &value, &flag), 4);
  ASSERT_THAT(value, Varint::Limits::N4_MIN_UINT_WITH_FLAG);
  ASSERT_THAT(flag, false);
}

TEST_F(VarintTestFixture, ReadN4MaxValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlag(Varint::Limits::N4_MAX_UINT_WITH_FLAG, true, b5,
                            sizeof b5);
  ASSERT_THAT(Varint::readUintWithFlag(b5, sizeof b5, &value, &flag), 4);
  ASSERT_THAT(value, Varint::Limits::N4_MAX_UINT_WITH_FLAG);
  ASSERT_THAT(flag, true);
}

TEST_F(VarintTestFixture, ReadN4MaxValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlag(Varint::Limits::N4_MAX_UINT_WITH_FLAG, false, b5,
                            sizeof b5);
  ASSERT_THAT(Varint::readUintWithFlag(b5, sizeof b5, &value, &flag), 4);
  ASSERT_THAT(value, Varint::Limits::N4_MAX_UINT_WITH_FLAG);
  ASSERT_THAT(flag, false);
}

TEST_F(VarintTestFixture, ReadN5MinValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlag(Varint::Limits::N5_MIN_UINT_WITH_FLAG, true, b5,
                            sizeof b5);
  ASSERT_THAT(Varint::readUintWithFlag(b5, sizeof b5, &value, &flag), 5);
  ASSERT_THAT(value, Varint::Limits::N5_MIN_UINT_WITH_FLAG);
  ASSERT_THAT(flag, true);
}

TEST_F(VarintTestFixture, ReadN5MinValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlag(Varint::Limits::N5_MIN_UINT_WITH_FLAG, false, b5,
                            sizeof b5);
  ASSERT_THAT(Varint::readUintWithFlag(b5, sizeof b5, &value, &flag), 5);
  ASSERT_THAT(value, Varint::Limits::N5_MIN_UINT_WITH_FLAG);
  ASSERT_THAT(flag, false);
}

TEST_F(VarintTestFixture, ReadN5MaxValueWithTrueFlag) {
  flag = false;
  Varint::writeUintWithFlag(Varint::Limits::N5_MAX_UINT_WITH_FLAG, true, b5,
                            sizeof b5);
  ASSERT_THAT(Varint::readUintWithFlag(b5, sizeof b5, &value, &flag), 5);
  ASSERT_THAT(value, Varint::Limits::N5_MAX_UINT_WITH_FLAG);
  ASSERT_THAT(flag, true);
}

TEST_F(VarintTestFixture, ReadN5MaxValueWithFalseFlag) {
  flag = true;
  Varint::writeUintWithFlag(Varint::Limits::N5_MAX_UINT_WITH_FLAG, false, b5,
                            sizeof b5);
  ASSERT_THAT(Varint::readUintWithFlag(b5, sizeof b5, &value, &flag), 5);
  ASSERT_THAT(value, Varint::Limits::N5_MAX_UINT_WITH_FLAG);
  ASSERT_THAT(flag, false);
}

TEST_F(VarintTestFixture, WriteAndReadSomeValues) {
  std::uint32_t values[] = {
    (Varint::Limits::N1_MAX_UINT - Varint::Limits::N1_MIN_UINT) / 2,
    (Varint::Limits::N2_MAX_UINT - Varint::Limits::N2_MIN_UINT) / 2,
    (Varint::Limits::N3_MAX_UINT - Varint::Limits::N3_MIN_UINT) / 2,
    (Varint::Limits::N4_MAX_UINT - Varint::Limits::N4_MIN_UINT) / 2,
    (Varint::Limits::N5_MAX_UINT - Varint::Limits::N5_MIN_UINT) / 2
  };
  auto p = b32;
  p += Varint::writeUint(values[0], p, sizeof b32 - (p - b32));
  p += Varint::writeUint(values[1], p, sizeof b32 - (p - b32));
  p += Varint::writeUint(values[2], p, sizeof b32 - (p - b32));
  p += Varint::writeUint(values[3], p, sizeof b32 - (p - b32));
  p += Varint::writeUint(values[4], p, sizeof b32 - (p - b32));
  ASSERT_THAT(p - b32, 15);

  p = b32;
  p += Varint::readUint(p, sizeof b32 - (p - b32), &value);
  ASSERT_THAT(value, values[0]);
  p += Varint::readUint(p, sizeof b32 - (p - b32), &value);
  ASSERT_THAT(value, values[1]);
  p += Varint::readUint(p, sizeof b32 - (p - b32), &value);
  ASSERT_THAT(value, values[2]);
  p += Varint::readUint(p, sizeof b32 - (p - b32), &value);
  ASSERT_THAT(value, values[3]);
  p += Varint::readUint(p, sizeof b32 - (p - b32), &value);
  ASSERT_THAT(value, values[4]);
  ASSERT_THAT(p - b32, 15);
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
                                2,
                            (Varint::Limits::N5_MAX_UINT_WITH_FLAG -
                             Varint::Limits::N5_MIN_UINT_WITH_FLAG) /
                                2 };
  auto p = b32;
  p += Varint::writeUintWithFlag(values[0], true, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[1], true, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[2], true, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[3], true, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[4], true, p, sizeof b32 - (p - b32));
  ASSERT_THAT(p - b32, 15);

  p = b32;
  flag = false;
  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_THAT(value, values[0]);
  ASSERT_THAT(flag, true);

  flag = false;
  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_THAT(value, values[1]);
  ASSERT_THAT(flag, true);

  flag = false;
  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_THAT(value, values[2]);
  ASSERT_THAT(flag, true);

  flag = false;
  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_THAT(value, values[3]);
  ASSERT_THAT(flag, true);

  flag = false;
  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_THAT(value, values[4]);
  ASSERT_THAT(flag, true);

  ASSERT_THAT(p - b32, 15);
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
                                2,
                            (Varint::Limits::N5_MAX_UINT_WITH_FLAG -
                             Varint::Limits::N5_MIN_UINT_WITH_FLAG) /
                                2 };
  auto p = b32;
  p += Varint::writeUintWithFlag(values[0], false, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[1], false, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[2], false, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[3], false, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[4], false, p, sizeof b32 - (p - b32));
  ASSERT_THAT(p - b32, 15);

  p = b32;
  flag = true;
  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_THAT(value, values[0]);
  ASSERT_THAT(flag, false);

  flag = true;
  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_THAT(value, values[1]);
  ASSERT_THAT(flag, false);

  flag = true;
  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_THAT(value, values[2]);
  ASSERT_THAT(flag, false);

  flag = true;
  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_THAT(value, values[3]);
  ASSERT_THAT(flag, false);

  flag = true;
  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_THAT(value, values[4]);
  ASSERT_THAT(flag, false);

  ASSERT_THAT(p - b32, 15);
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
                                2,
                            (Varint::Limits::N5_MAX_UINT_WITH_FLAG -
                             Varint::Limits::N5_MIN_UINT_WITH_FLAG) /
                                2 };
  auto p = b32;
  p += Varint::writeUintWithFlag(values[0], true, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[1], false, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[2], true, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[3], false, p, sizeof b32 - (p - b32));
  p += Varint::writeUintWithFlag(values[4], true, p, sizeof b32 - (p - b32));
  ASSERT_THAT(p - b32, 15);

  p = b32;
  flag = false;
  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_THAT(value, values[0]);
  ASSERT_THAT(flag, true);

  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_THAT(value, values[1]);
  ASSERT_THAT(flag, false);

  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_THAT(value, values[2]);
  ASSERT_THAT(flag, true);

  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_THAT(value, values[3]);
  ASSERT_THAT(flag, false);

  p += Varint::readUintWithFlag(p, sizeof b32 - (p - b32), &value, &flag);
  ASSERT_THAT(value, values[4]);
  ASSERT_THAT(flag, true);

  ASSERT_THAT(p - b32, 15);
}

} // namespace internal
} // namespace multimap
