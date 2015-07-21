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

using testing::Eq;

TEST(VarintTest, IsNotDefaultConstructible) {
  ASSERT_THAT(std::is_default_constructible<Varint>::value, Eq(false));
}

TEST(VarintTest, WriteValueToNullPtrAndDie) {
  ASSERT_DEATH(Varint::WriteUint32(0, nullptr), "");
}

TEST(VarintTest, WriteMinValueEncodedInOneByte) {
  byte buf[4];
  ASSERT_THAT(Varint::WriteUint32(Varint::kMinValueStoredIn8Bits, buf), Eq(1));
}

TEST(VarintTest, WriteMaxValueEncodedInOneByte) {
  byte buf[4];
  ASSERT_THAT(Varint::WriteUint32(Varint::kMaxValueStoredIn8Bits, buf), Eq(1));
}

TEST(VarintTest, WriteMinValueEncodedInTwoBytes) {
  byte buf[4];
  ASSERT_THAT(Varint::WriteUint32(Varint::kMinValueStoredIn16Bits, buf), Eq(2));
}

TEST(VarintTest, WriteMaxValueEncodedInTwoBytes) {
  byte buf[4];
  ASSERT_THAT(Varint::WriteUint32(Varint::kMaxValueStoredIn16Bits, buf), Eq(2));
}

TEST(VarintTest, WriteMinValueEncodedInThreeBytes) {
  byte buf[4];
  ASSERT_THAT(Varint::WriteUint32(Varint::kMinValueStoredIn24Bits, buf), Eq(3));
}

TEST(VarintTest, WriteMaxValueEncodedInThreeBytes) {
  byte buf[4];
  ASSERT_THAT(Varint::WriteUint32(Varint::kMaxValueStoredIn24Bits, buf), Eq(3));
}

TEST(VarintTest, WriteMinValueEncodedInFourBytes) {
  byte buf[4];
  ASSERT_THAT(Varint::WriteUint32(Varint::kMinValueStoredIn32Bits, buf), Eq(4));
}

TEST(VarintTest, WriteMaxValueEncodedInFourBytes) {
  byte buf[4];
  ASSERT_THAT(Varint::WriteUint32(Varint::kMaxValueStoredIn32Bits, buf), Eq(4));
}

TEST(VarintTest, WriteTooBigValueAndThrow) {
  byte buf[4];
  ASSERT_ANY_THROW(
      Varint::WriteUint32(Varint::kMaxValueStoredIn32Bits + 1, buf));
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
  Varint::WriteUint32(Varint::kMinValueStoredIn8Bits, buf);
  std::uint32_t target;
  ASSERT_THAT(Varint::ReadUint32(buf, &target), Eq(1));
  ASSERT_THAT(target, Eq(Varint::kMinValueStoredIn8Bits));
}

TEST(VarintTest, ReadMaxValueEncodedInOneByte) {
  byte buf[4];
  Varint::WriteUint32(Varint::kMaxValueStoredIn8Bits, buf);
  std::uint32_t target;
  ASSERT_THAT(Varint::ReadUint32(buf, &target), Eq(1));
  ASSERT_THAT(target, Eq(Varint::kMaxValueStoredIn8Bits));
}

TEST(VarintTest, ReadMinValueEncodedInTwoBytes) {
  byte buf[4];
  Varint::WriteUint32(Varint::kMinValueStoredIn16Bits, buf);
  std::uint32_t target;
  ASSERT_THAT(Varint::ReadUint32(buf, &target), Eq(2));
  ASSERT_THAT(target, Eq(Varint::kMinValueStoredIn16Bits));
}

TEST(VarintTest, ReadMaxValueEncodedInTwoBytes) {
  byte buf[4];
  Varint::WriteUint32(Varint::kMaxValueStoredIn16Bits, buf);
  std::uint32_t target;
  ASSERT_THAT(Varint::ReadUint32(buf, &target), Eq(2));
  ASSERT_THAT(target, Eq(Varint::kMaxValueStoredIn16Bits));
}

TEST(VarintTest, ReadMinValueEncodedInThreeBytes) {
  byte buf[4];
  Varint::WriteUint32(Varint::kMinValueStoredIn24Bits, buf);
  std::uint32_t target;
  ASSERT_THAT(Varint::ReadUint32(buf, &target), Eq(3));
  ASSERT_THAT(target, Eq(Varint::kMinValueStoredIn24Bits));
}

TEST(VarintTest, ReadMaxValueEncodedInThreeBytes) {
  byte buf[4];
  Varint::WriteUint32(Varint::kMaxValueStoredIn24Bits, buf);
  std::uint32_t target;
  ASSERT_THAT(Varint::ReadUint32(buf, &target), Eq(3));
  ASSERT_THAT(target, Eq(Varint::kMaxValueStoredIn24Bits));
}

TEST(VarintTest, ReadMinValueEncodedInFourBytes) {
  byte buf[4];
  Varint::WriteUint32(Varint::kMinValueStoredIn32Bits, buf);
  std::uint32_t target;
  ASSERT_THAT(Varint::ReadUint32(buf, &target), Eq(4));
  ASSERT_THAT(target, Eq(Varint::kMinValueStoredIn32Bits));
}

TEST(VarintTest, ReadMaxValueEncodedInFourBytes) {
  byte buf[4];
  Varint::WriteUint32(Varint::kMaxValueStoredIn32Bits, buf);
  std::uint32_t target;
  ASSERT_THAT(Varint::ReadUint32(buf, &target), Eq(4));
  ASSERT_THAT(target, Eq(Varint::kMaxValueStoredIn32Bits));
}

TEST(VarintTest, WriteAndReadSomeValuesOfDifferentSize) {
  std::uint32_t values[] = {
      (Varint::kMaxValueStoredIn8Bits - Varint::kMinValueStoredIn8Bits) /
          2,  // Needs 1 byte
      (Varint::kMaxValueStoredIn16Bits - Varint::kMinValueStoredIn16Bits) /
          2,  // Needs 2 bytes
      (Varint::kMaxValueStoredIn24Bits - Varint::kMinValueStoredIn24Bits) /
          2,  // Needs 3 bytes
      (Varint::kMaxValueStoredIn32Bits - Varint::kMinValueStoredIn32Bits) /
          2  // Needs 4 bytes
  };
  byte buf[32];
  auto pos = buf;
  pos += Varint::WriteUint32(values[0], pos);
  pos += Varint::WriteUint32(values[1], pos);
  pos += Varint::WriteUint32(values[2], pos);
  pos += Varint::WriteUint32(values[3], pos);
  ASSERT_THAT(std::distance(buf, pos), Eq(10));

  pos = buf;
  std::uint32_t target;
  pos += Varint::ReadUint32(pos, &target);
  ASSERT_THAT(target, Eq(values[0]));
  pos += Varint::ReadUint32(pos, &target);
  ASSERT_THAT(target, Eq(values[1]));
  pos += Varint::ReadUint32(pos, &target);
  ASSERT_THAT(target, Eq(values[2]));
  pos += Varint::ReadUint32(pos, &target);
  ASSERT_THAT(target, Eq(values[3]));
  ASSERT_THAT(std::distance(buf, pos), Eq(10));
}

}  // namespace internal
}  // namespace multimap
