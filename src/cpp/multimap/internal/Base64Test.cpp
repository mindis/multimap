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

#include <string>
#include <type_traits>
#include "gmock/gmock.h"
#include "multimap/internal/Base64.h"

namespace multimap {
namespace internal {

// https://en.wikipedia.org/wiki/Base64
const char* TEST_STRING_A_BINARY = "any carnal pleasure.";
const char* TEST_STRING_A_BASE64 = "YW55IGNhcm5hbCBwbGVhc3VyZS4=";

const char* TEST_STRING_B_BINARY = "any carnal pleasure";
const char* TEST_STRING_B_BASE64 = "YW55IGNhcm5hbCBwbGVhc3VyZQ==";

const char* TEST_STRING_C_BINARY = "any carnal pleasur";
const char* TEST_STRING_C_BASE64 = "YW55IGNhcm5hbCBwbGVhc3Vy";

const char* TEST_STRING_D_BINARY = "any carnal pleasu";
const char* TEST_STRING_D_BASE64 = "YW55IGNhcm5hbCBwbGVhc3U=";

const char* TEST_STRING_E_BINARY = "any carnal pleas";
const char* TEST_STRING_E_BASE64 = "YW55IGNhcm5hbCBwbGVhcw==";

const char* TEST_STRING_F_BINARY =
    "Man is distinguished, not only by his reason, but by this singular "
    "passion from other animals, which is a lust of the mind, that by a "
    "perseverance of delight in the continued and indefatigable generation of "
    "knowledge, exceeds the short vehemence of any carnal pleasure.";

const char* TEST_STRING_F_BASE64 =
    "TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aG"
    "lzIHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qg"
    "b2YgdGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY2"
    "9udGludWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNl"
    "ZWRzIHRoZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=";

TEST(Base64Test, IsNotDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<Base64>::value);
}

TEST(Base64Test, EncodeZeroBytesReturnsEmptyString) {
  Slice bytes;
  std::string base64;
  Base64::encode(bytes, &base64);
  ASSERT_TRUE(base64.empty());

  base64 = "nonempty";
  Base64::encode(bytes, &base64);
  ASSERT_TRUE(base64.empty());
}

TEST(Base64Test, EncodeTestStringA) {
  std::string base64;
  Base64::encode(TEST_STRING_A_BINARY, &base64);
  ASSERT_EQ(base64, TEST_STRING_A_BASE64);
}

TEST(Base64Test, EncodeTestStringB) {
  std::string base64;
  Base64::encode(TEST_STRING_B_BINARY, &base64);
  ASSERT_EQ(base64, TEST_STRING_B_BASE64);
}

TEST(Base64Test, EncodeTestStringC) {
  std::string base64;
  Base64::encode(TEST_STRING_C_BINARY, &base64);
  ASSERT_EQ(base64, TEST_STRING_C_BASE64);
}

TEST(Base64Test, EncodeTestStringD) {
  std::string base64;
  Base64::encode(TEST_STRING_D_BINARY, &base64);
  ASSERT_EQ(base64, TEST_STRING_D_BASE64);
}

TEST(Base64Test, EncodeTestStringE) {
  std::string base64;
  Base64::encode(TEST_STRING_E_BINARY, &base64);
  ASSERT_EQ(base64, TEST_STRING_E_BASE64);
}

TEST(Base64Test, EncodeTestStringF) {
  std::string base64;
  Base64::encode(TEST_STRING_F_BINARY, &base64);
  ASSERT_EQ(base64, TEST_STRING_F_BASE64);
}

TEST(Base64Test, DecodeEmptyStringReturnsZeroBytes) {
  Bytes bytes;
  Base64::decode("", &bytes);
  ASSERT_TRUE(bytes.empty());

  bytes = toBytes("old content");
  Base64::decode("", &bytes);
  ASSERT_TRUE(bytes.empty());
}

TEST(Base64Test, DecodeTestStringA) {
  Bytes bytes;
  Base64::decode(TEST_STRING_A_BASE64, &bytes);
  ASSERT_EQ(bytes, Slice(TEST_STRING_A_BINARY));
}

TEST(Base64Test, DecodeTestStringB) {
  Bytes bytes;
  Base64::decode(TEST_STRING_B_BASE64, &bytes);
  ASSERT_EQ(bytes, Slice(TEST_STRING_B_BINARY));
}

TEST(Base64Test, DecodeTestStringC) {
  Bytes bytes;
  Base64::decode(TEST_STRING_C_BASE64, &bytes);
  ASSERT_EQ(bytes, Slice(TEST_STRING_C_BINARY));
}

TEST(Base64Test, DecodeTestStringD) {
  Bytes bytes;
  Base64::decode(TEST_STRING_D_BASE64, &bytes);
  ASSERT_EQ(bytes, Slice(TEST_STRING_D_BINARY));
}

TEST(Base64Test, DecodeTestStringE) {
  Bytes bytes;
  Base64::decode(TEST_STRING_E_BASE64, &bytes);
  ASSERT_EQ(bytes, Slice(TEST_STRING_E_BINARY));
}

TEST(Base64Test, DecodeTestStringF) {
  Bytes bytes;
  Base64::decode(TEST_STRING_F_BASE64, &bytes);
  ASSERT_EQ(bytes, Slice(TEST_STRING_F_BINARY));
}

}  // namespace internal
}  // namespace multimap
