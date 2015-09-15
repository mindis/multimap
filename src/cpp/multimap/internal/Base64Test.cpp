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

#include <string>
#include "gmock/gmock.h"
#include "multimap/internal/Base64.hpp"

namespace multimap {
namespace internal {

// https://en.wikipedia.org/wiki/Base64
const std::string kTestStringA_Binary = "any carnal pleasure.";
const std::string kTestStringA_Base64 = "YW55IGNhcm5hbCBwbGVhc3VyZS4=";

const std::string kTestStringB_Binary = "any carnal pleasure";
const std::string kTestStringB_Base64 = "YW55IGNhcm5hbCBwbGVhc3VyZQ==";

const std::string kTestStringC_Binary = "any carnal pleasur";
const std::string kTestStringC_Base64 = "YW55IGNhcm5hbCBwbGVhc3Vy";

const std::string kTestStringD_Binary = "any carnal pleasu";
const std::string kTestStringD_Base64 = "YW55IGNhcm5hbCBwbGVhc3U=";

const std::string kTestStringE_Binary = "any carnal pleas";
const std::string kTestStringE_Base64 = "YW55IGNhcm5hbCBwbGVhcw==";

const std::string kTestStringF_Binary =
    "Man is distinguished, not only by his reason, but by this singular "
    "passion from other animals, which is a lust of the mind, that by a "
    "perseverance of delight in the continued and indefatigable generation of "
    "knowledge, exceeds the short vehemence of any carnal pleasure.";

const std::string kTestStringF_Base64 =
    "TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aG"
    "lzIHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qg"
    "b2YgdGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY2"
    "9udGludWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNl"
    "ZWRzIHRoZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=";

TEST(Base64Test, EncodeZeroBytesReturnsEmptyString) {
  Bytes binary;
  std::string base64;
  Base64::encode(binary, &base64);
  ASSERT_TRUE(base64.empty());

  base64 = "nonempty";
  Base64::encode(binary, &base64);
  ASSERT_TRUE(base64.empty());
}

TEST(Base64Test, EncodeTestStringA) {
  std::string base64;
  Base64::encode(kTestStringA_Binary, &base64);
  ASSERT_EQ(base64, kTestStringA_Base64);
}

TEST(Base64Test, EncodeTestStringB) {
  std::string base64;
  Base64::encode(kTestStringB_Binary, &base64);
  ASSERT_EQ(base64, kTestStringB_Base64);
}

TEST(Base64Test, EncodeTestStringC) {
  std::string base64;
  Base64::encode(kTestStringC_Binary, &base64);
  ASSERT_EQ(base64, kTestStringC_Base64);
}

TEST(Base64Test, EncodeTestStringD) {
  std::string base64;
  Base64::encode(kTestStringD_Binary, &base64);
  ASSERT_EQ(base64, kTestStringD_Base64);
}

TEST(Base64Test, EncodeTestStringE) {
  std::string base64;
  Base64::encode(kTestStringE_Binary, &base64);
  ASSERT_EQ(base64, kTestStringE_Base64);
}

TEST(Base64Test, EncodeTestStringF) {
  std::string base64;
  Base64::encode(kTestStringF_Binary, &base64);
  ASSERT_EQ(base64, kTestStringF_Base64);
}

TEST(Base64Test, DecodeEmptyStringReturnsZeroBytes) {
  std::string base64;
  std::string binary;
  Base64::decode(base64, &binary);
  ASSERT_TRUE(binary.empty());

  binary = "\x12\x34\x56";
  Base64::decode(base64, &binary);
  ASSERT_TRUE(binary.empty());
}

TEST(Base64Test, DecodeTestStringA) {
  std::string binary;
  Base64::decode(kTestStringA_Base64, &binary);
  ASSERT_EQ(binary, kTestStringA_Binary);
}

TEST(Base64Test, DecodeTestStringB) {
  std::string binary;
  Base64::decode(kTestStringB_Base64, &binary);
  ASSERT_EQ(binary, kTestStringB_Binary);
}

TEST(Base64Test, DecodeTestStringC) {
  std::string binary;
  Base64::decode(kTestStringC_Base64, &binary);
  ASSERT_EQ(binary, kTestStringC_Binary);
}

TEST(Base64Test, DecodeTestStringD) {
  std::string binary;
  Base64::decode(kTestStringD_Base64, &binary);
  ASSERT_EQ(binary, kTestStringD_Binary);
}

TEST(Base64Test, DecodeTestStringE) {
  std::string binary;
  Base64::decode(kTestStringE_Base64, &binary);
  ASSERT_EQ(binary, kTestStringE_Binary);
}

TEST(Base64Test, DecodeTestStringF) {
  std::string binary;
  Base64::decode(kTestStringF_Base64, &binary);
  ASSERT_EQ(binary, kTestStringF_Binary);
}

}  // namespace internal
}  // namespace multimap
