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

#include <type_traits>
#include "gmock/gmock.h"
#include "multimap/internal/Varint.hpp"

namespace multimap {
namespace internal {

TEST(VarintTest, IsNotDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<Varint>::value);
}

TEST(VarintTest, WriteValueToNullPtrAndDie) {
  ASSERT_DEATH(Varint::WriteUint32(0, nullptr), "");
}

TEST(VarintTest, WriteMinValueEncodedInOneByte) {
  byte buf[4];
  ASSERT_EQ(Varint::WriteUint32(Varint::min_value_1_byte(), buf), 1);
}

TEST(VarintTest, WriteMaxValueEncodedInOneByte) {
  byte buf[4];
  ASSERT_EQ(Varint::WriteUint32(Varint::max_value_1_byte(), buf), 1);
}

TEST(VarintTest, WriteMinValueEncodedInTwoBytes) {
  byte buf[4];
  ASSERT_EQ(Varint::WriteUint32(Varint::min_value_2_bytes(), buf), 2);
}

TEST(VarintTest, WriteMaxValueEncodedInTwoBytes) {
  byte buf[4];
  ASSERT_EQ(Varint::WriteUint32(Varint::max_value_2_bytes(), buf), 2);
}

TEST(VarintTest, WriteMinValueEncodedInThreeBytes) {
  byte buf[4];
  ASSERT_EQ(Varint::WriteUint32(Varint::min_value_3_bytes(), buf), 3);
}

TEST(VarintTest, WriteMaxValueEncodedInThreeBytes) {
  byte buf[4];
  ASSERT_EQ(Varint::WriteUint32(Varint::max_value_3_bytes(), buf), 3);
}

TEST(VarintTest, WriteMinValueEncodedInFourBytes) {
  byte buf[4];
  ASSERT_EQ(Varint::WriteUint32(Varint::min_value_4_bytes(), buf), 4);
}

TEST(VarintTest, WriteMaxValueEncodedInFourBytes) {
  byte buf[4];
  ASSERT_EQ(Varint::WriteUint32(Varint::max_value_4_bytes(), buf), 4);
}

TEST(VarintTest, WriteTooBigValueAndThrow) {
  byte buf[4];
  ASSERT_ANY_THROW(Varint::WriteUint32(Varint::max_value_4_bytes() + 1, buf));
}

TEST(VarintTest, ReadValueFromNullPtrAndDie) {
  std::uint32_t target;
  ASSERT_DEATH(Varint::ReadUint32(nullptr, &target), "");
}

TEST(VarintTest, ReadValueIntoNullPtrAndDie) {
  byte buf[4];
  ASSERT_DEATH(Varint::ReadUint32(buf, nullptr), "");
}

TEST(VarintTest, ReadMinValueEncodedInOneByte) {
  byte buf[4];
  Varint::WriteUint32(Varint::min_value_1_byte(), buf);
  std::uint32_t target;
  ASSERT_EQ(Varint::ReadUint32(buf, &target), 1);
  ASSERT_EQ(target, Varint::min_value_1_byte());
}

TEST(VarintTest, ReadMaxValueEncodedInOneByte) {
  byte buf[4];
  Varint::WriteUint32(Varint::max_value_1_byte(), buf);
  std::uint32_t target;
  ASSERT_EQ(Varint::ReadUint32(buf, &target), 1);
  ASSERT_EQ(target, Varint::max_value_1_byte());
}

TEST(VarintTest, ReadMinValueEncodedInTwoBytes) {
  byte buf[4];
  Varint::WriteUint32(Varint::min_value_2_bytes(), buf);
  std::uint32_t target;
  ASSERT_EQ(Varint::ReadUint32(buf, &target), 2);
  ASSERT_EQ(target, Varint::min_value_2_bytes());
}

TEST(VarintTest, ReadMaxValueEncodedInTwoBytes) {
  byte buf[4];
  Varint::WriteUint32(Varint::max_value_2_bytes(), buf);
  std::uint32_t target;
  ASSERT_EQ(Varint::ReadUint32(buf, &target), 2);
  ASSERT_EQ(target, Varint::max_value_2_bytes());
}

TEST(VarintTest, ReadMinValueEncodedInThreeBytes) {
  byte buf[4];
  Varint::WriteUint32(Varint::min_value_3_bytes(), buf);
  std::uint32_t target;
  ASSERT_EQ(Varint::ReadUint32(buf, &target), 3);
  ASSERT_EQ(target, Varint::min_value_3_bytes());
}

TEST(VarintTest, ReadMaxValueEncodedInThreeBytes) {
  byte buf[4];
  Varint::WriteUint32(Varint::max_value_3_bytes(), buf);
  std::uint32_t target;
  ASSERT_EQ(Varint::ReadUint32(buf, &target), 3);
  ASSERT_EQ(target, Varint::max_value_3_bytes());
}

TEST(VarintTest, ReadMinValueEncodedInFourBytes) {
  byte buf[4];
  Varint::WriteUint32(Varint::min_value_4_bytes(), buf);
  std::uint32_t target;
  ASSERT_EQ(Varint::ReadUint32(buf, &target), 4);
  ASSERT_EQ(target, Varint::min_value_4_bytes());
}

TEST(VarintTest, ReadMaxValueEncodedInFourBytes) {
  byte buf[4];
  Varint::WriteUint32(Varint::max_value_4_bytes(), buf);
  std::uint32_t target;
  ASSERT_EQ(Varint::ReadUint32(buf, &target), 4);
  ASSERT_EQ(target, Varint::max_value_4_bytes());
}

TEST(VarintTest, WriteAndReadSomeValuesOfDifferentSize) {
  std::uint32_t values[] = {
      (Varint::max_value_1_byte() - Varint::min_value_1_byte()) / 2,
      (Varint::max_value_2_bytes() - Varint::min_value_2_bytes()) / 2,
      (Varint::max_value_3_bytes() - Varint::min_value_3_bytes()) / 2,
      (Varint::max_value_4_bytes() - Varint::min_value_4_bytes()) / 2};
  byte buf[32];
  auto pos = buf;
  pos += Varint::WriteUint32(values[0], pos);
  pos += Varint::WriteUint32(values[1], pos);
  pos += Varint::WriteUint32(values[2], pos);
  pos += Varint::WriteUint32(values[3], pos);
  ASSERT_EQ(std::distance(buf, pos), 10);

  pos = buf;
  std::uint32_t target;
  pos += Varint::ReadUint32(pos, &target);
  ASSERT_EQ(target, values[0]);
  pos += Varint::ReadUint32(pos, &target);
  ASSERT_EQ(target, values[1]);
  pos += Varint::ReadUint32(pos, &target);
  ASSERT_EQ(target, values[2]);
  pos += Varint::ReadUint32(pos, &target);
  ASSERT_EQ(target, values[3]);
  ASSERT_EQ(std::distance(buf, pos), 10);
}

}  // namespace internal
}  // namespace multimap
