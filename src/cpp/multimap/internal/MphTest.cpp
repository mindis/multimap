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

#include <type_traits>
#include <vector>
#include <boost/filesystem/operations.hpp>
#include "multimap/internal/Mph.hpp"
#include "multimap/thirdparty/mt/assert.hpp"
#include "multimap/thirdparty/mt/fileio.hpp"
#include "multimap/Arena.hpp"
#include "gmock/gmock.h"

namespace multimap {
namespace internal {

TEST(MphTest, IsNotDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<Mph>::value);
}

TEST(MphTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<Mph>::value);
  ASSERT_FALSE(std::is_copy_assignable<Mph>::value);
}

TEST(MphTest, IsMoveConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_move_constructible<Mph>::value);
  ASSERT_TRUE(std::is_move_assignable<Mph>::value);
}

// -----------------------------------------------------------------------------
// Generation and Evaluation
// -----------------------------------------------------------------------------

std::string makeKey(int index) { return std::to_string(index); }

byte* makeCmphEncodedKey(int index, Arena* arena) {
  const std::string key = makeKey(index);
  const uint32_t key_size = key.size();
  byte* full_key = arena->allocate(sizeof key_size + key_size);
  std::memcpy(full_key, &key_size, sizeof key_size);
  std::memcpy(full_key + sizeof key_size, key.data(), key_size);
  return full_key;
}

void writeCmphEncodedKey(int index, std::FILE* stream) {
  const std::string key = makeKey(index);
  const uint32_t key_size = key.size();
  mt::fwriteAll(stream, &key_size, sizeof key_size);
  mt::fwriteAll(stream, key.data(), key_size);
}

TEST(MphTest, BuildFromVerySmallKeysetThrows) {
  // CMPH is not suitable for very small keysets.
  Arena arena;
  std::vector<const byte*> keys(2);
  for (int i = 0; i < keys.size(); i++) {
    keys[i] = makeCmphEncodedKey(i, &arena);
  }
  ASSERT_THROW(Mph::build(keys.data(), keys.size()), std::runtime_error);
}

class MphTestWithParam : public testing::TestWithParam<int> {
 public:
  void SetUp() override {
    directory_ = "/tmp/multimap.MphTestWithParam";
    boost::filesystem::remove_all(directory_);
    MT_ASSERT_TRUE(boost::filesystem::create_directory(directory_));
    keys_file_ = directory_ / "keys";
    mph_file_ = directory_ / "mph";
  }

  void TearDown() override {
    MT_ASSERT_TRUE(boost::filesystem::remove_all(directory_));
  }

  const boost::filesystem::path& getKeysFilename() { return keys_file_; }
  const boost::filesystem::path& getMphFilename() { return mph_file_; }

 private:
  boost::filesystem::path directory_;
  boost::filesystem::path keys_file_;
  boost::filesystem::path mph_file_;
};

TEST_P(MphTestWithParam, BuildFromInMemoryKeys) {
  Arena arena;
  std::vector<const byte*> keys(GetParam());
  for (int i = 0; i < keys.size(); i++) {
    keys[i] = makeCmphEncodedKey(i, &arena);
  }
  const Mph mph = Mph::build(keys.data(), keys.size());
  ASSERT_EQ(keys.size(), mph.size());

  for (int i = 0; i < keys.size(); i++) {
    const std::string key = makeKey(i);
    ASSERT_LT(mph(key), mph.size());
  }
}

TEST_P(MphTestWithParam, BuildFromOnDiskKeys) {
  mt::AutoCloseFile stream = mt::fopen(getKeysFilename().string(), "w");
  for (int i = 0; i < GetParam(); i++) {
    writeCmphEncodedKey(i, stream.get());
  }
  stream.reset();  // Closes the file.
  const Mph mph = Mph::build(getKeysFilename());
  ASSERT_EQ(GetParam(), mph.size());

  for (int i = 0; i < GetParam(); i++) {
    const std::string key = makeKey(i);
    ASSERT_LT(mph(key), mph.size());
  }
}

INSTANTIATE_TEST_CASE_P(Parameterized, MphTestWithParam,
                        testing::Values(10, 1000, 1000000, 10000000));
// CMPH does not work for very small keysets, i.e. less than 10.

TEST(MphTest, HashValueForUnknownKeyIsInRange) {
  Arena arena;
  std::vector<const byte*> keys(1000);
  for (int i = 0; i < keys.size(); i++) {
    keys[i] = makeCmphEncodedKey(i, &arena);
  }
  const Mph mph = Mph::build(keys.data(), keys.size());
  ASSERT_EQ(keys.size(), mph.size());

  const int limit = keys.size() * 2;
  for (int i = keys.size(); i < limit; i++) {
    const std::string key = makeKey(i);
    ASSERT_LT(mph(key), mph.size());
  }
}

// -----------------------------------------------------------------------------
// Serialization
// -----------------------------------------------------------------------------

TEST_P(MphTestWithParam, WriteMphToFileThenReadBackAndEvaluate) {
  Arena arena;
  std::vector<const byte*> keys(GetParam());
  for (int i = 0; i < keys.size(); i++) {
    keys[i] = makeCmphEncodedKey(i, &arena);
  }
  Mph::build(keys.data(), keys.size()).writeToFile(getMphFilename());

  const Mph mph = Mph::readFromFile(getMphFilename());
  for (int i = 0; i < keys.size(); i++) {
    const std::string key = makeKey(i);
    ASSERT_LT(mph(key), mph.size());
  }
}

}  // namespace internal
}  // namespace multimap
