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

#include <thread>
#include <type_traits>
#include <boost/filesystem/operations.hpp>
#include "gmock/gmock.h"
#include "multimap/internal/Shard.hpp"
#include "multimap/callables.hpp"

namespace multimap {
namespace internal {

using testing::Eq;
using testing::ElementsAre;
using testing::UnorderedElementsAre;

std::unique_ptr<Shard> openOrCreateShard(
    const boost::filesystem::path& prefix) {
  Shard::Options options;
  options.create_if_missing = true;
  return std::unique_ptr<Shard>(new Shard(prefix, options));
}

std::unique_ptr<Shard> openOrCreateShardAsReadOnly(
    const boost::filesystem::path& prefix) {
  Shard::Options options;
  options.readonly = true;
  options.create_if_missing = true;
  return std::unique_ptr<Shard>(new Shard(prefix, options));
}

TEST(ShardTest, IsNotDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<Shard>::value);
}

TEST(ShardTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<Shard>::value);
  ASSERT_FALSE(std::is_copy_assignable<Shard>::value);
}

TEST(ShardTest, IsNotMoveConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_move_constructible<Shard>::value);
  ASSERT_FALSE(std::is_move_assignable<Shard>::value);
}

struct ShardTestFixture : public testing::Test {
  void SetUp() override {
    boost::filesystem::remove_all(directory);
    boost::filesystem::create_directory(directory);
  }

  void TearDown() override { boost::filesystem::remove_all(directory); }

  const boost::filesystem::path directory = "/tmp/multimap.ShardTestFixture";
  const boost::filesystem::path prefix = directory / "shard";
  const std::string k1 = "k1";
  const std::string k2 = "k2";
  const std::string k3 = "k3";
  const std::string v1 = "v1";
  const std::string v2 = "v2";
  const std::string v3 = "v3";
};

TEST_F(ShardTestFixture, ConstructorThrowsIfNotExist) {
  ASSERT_THROW(Shard(this->prefix), std::runtime_error);
  // GCC complains when not using 'this' pointer.
}

TEST_F(ShardTestFixture, ConstructorWithDefaultOptionsThrowsIfNotExist) {
  Shard::Options options;
  ASSERT_THROW(Shard(prefix, options), std::runtime_error);
}

TEST_F(ShardTestFixture, ConstructorWithCreateIfMissingDoesNotThrow) {
  Shard::Options options;
  options.create_if_missing = true;
  ASSERT_NO_THROW(Shard(prefix, options));
}

TEST_F(ShardTestFixture, PutAppendsValuesToList) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  shard->put(k1, v2);
  shard->put(k1, v3);
  auto iter = shard->get(k1);
  ASSERT_THAT(iter.next(), Eq(v1));
  ASSERT_THAT(iter.next(), Eq(v2));
  ASSERT_THAT(iter.next(), Eq(v3));
  ASSERT_FALSE(iter.hasNext());
}

TEST_F(ShardTestFixture, PutMaxKeyDoesNotThrow) {
  auto shard = openOrCreateShard(prefix);
  try {
    std::string key(Shard::Limits::maxKeySize(), 'k');
    ASSERT_NO_THROW(shard->put(key, v1));
  } catch (...) {
    // Allocating key itself may fail.
  }
}

TEST_F(ShardTestFixture, PutTooBigKeyThrows) {
  auto shard = openOrCreateShard(prefix);
  try {
    std::string key(Shard::Limits::maxKeySize() + 1, 'k');
    ASSERT_THROW(shard->put(key, v1), mt::AssertionError);
  } catch (...) {
    // Allocating key itself may fail.
  }
}

TEST_F(ShardTestFixture, PutMaxValueDoesNotThrow) {
  auto shard = openOrCreateShard(prefix);
  try {
    std::string value(Shard::Limits::maxValueSize(), 'v');
    ASSERT_NO_THROW(shard->put(k1, value));
  } catch (...) {
    // Allocating value itself may fail.
  }
}

TEST_F(ShardTestFixture, PutTooBigValueThrows) {
  auto shard = openOrCreateShard(prefix);
  try {
    std::string value(Shard::Limits::maxValueSize() + 1, 'v');
    ASSERT_THROW(shard->put(k1, value), mt::AssertionError);
  } catch (...) {
    // Allocating value itself may fail.
  }
}

TEST_F(ShardTestFixture, PutValuesAndReopenInBetween) {
  {
    auto shard = openOrCreateShard(prefix);
    shard->put(k1, v1);
    shard->put(k2, v1);
    shard->put(k3, v1);
  }
  {
    auto shard = openOrCreateShard(prefix);
    shard->put(k1, v2);
    shard->put(k2, v2);
    shard->put(k3, v2);
  }
  {
    auto shard = openOrCreateShard(prefix);
    shard->put(k1, v3);
    shard->put(k2, v3);
    shard->put(k3, v3);
  }
  auto shard = openOrCreateShard(prefix);
  auto iter1 = shard->get(k1);
  ASSERT_THAT(iter1.next(), Eq(v1));
  ASSERT_THAT(iter1.next(), Eq(v2));
  ASSERT_THAT(iter1.next(), Eq(v3));
  ASSERT_FALSE(iter1.hasNext());

  auto iter2 = shard->get(k2);
  ASSERT_THAT(iter2.next(), Eq(v1));
  ASSERT_THAT(iter2.next(), Eq(v2));
  ASSERT_THAT(iter2.next(), Eq(v3));
  ASSERT_FALSE(iter2.hasNext());

  auto iter3 = shard->get(k3);
  ASSERT_THAT(iter3.next(), Eq(v1));
  ASSERT_THAT(iter3.next(), Eq(v2));
  ASSERT_THAT(iter3.next(), Eq(v3));
  ASSERT_FALSE(iter3.hasNext());
}

TEST_F(ShardTestFixture, GetReturnsEmptyListForNonExistingKey) {
  auto shard = openOrCreateShard(prefix);
  ASSERT_FALSE(shard->get(k1).hasNext());
}

TEST_F(ShardTestFixture, RemoveKeyRemovesMappedValues) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  shard->put(k2, v1);
  shard->put(k3, v1);
  ASSERT_TRUE(shard->removeKey(k1));
  ASSERT_TRUE(shard->removeKey(k2));
  ASSERT_FALSE(shard->get(k1).hasNext());
  ASSERT_FALSE(shard->get(k2).hasNext());
  ASSERT_TRUE(shard->get(k3).hasNext());
}

TEST_F(ShardTestFixture, RemoveKeysRemovesMappedValues) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  shard->put(k2, v1);
  shard->put(k3, v1);
  auto is_k1_or_k2 = [&](const Bytes& key) { return key == k1 || key == k2; };
  ASSERT_THAT(shard->removeKeys(is_k1_or_k2), Eq(2));
  ASSERT_FALSE(shard->get(k1).hasNext());
  ASSERT_FALSE(shard->get(k2).hasNext());
  ASSERT_TRUE(shard->get(k3).hasNext());
}

TEST_F(ShardTestFixture, RemoveValueRemovesOnlyFirstMatch) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  shard->put(k1, v2);
  shard->put(k1, v3);
  auto is_any_value = [&](const Bytes& /* value */) { return true; };
  ASSERT_TRUE(shard->removeValue(k1, is_any_value));
  auto iter = shard->get(k1);
  ASSERT_THAT(iter.next(), Eq(v2));
  ASSERT_THAT(iter.next(), Eq(v3));
  ASSERT_FALSE(iter.hasNext());

  shard->put(k2, v1);
  shard->put(k2, v2);
  shard->put(k2, v3);
  auto is_v2_or_v3 =
      [&](const Bytes& value) { return value == v2 || value == v3; };
  ASSERT_TRUE(shard->removeValue(k2, is_v2_or_v3));
  iter = shard->get(k2);
  ASSERT_THAT(iter.next(), Eq(v1));
  ASSERT_THAT(iter.next(), Eq(v3));
  ASSERT_FALSE(iter.hasNext());
}

TEST_F(ShardTestFixture, RemoveValuesRemovesAllMatches) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  shard->put(k1, v2);
  shard->put(k1, v3);
  auto is_any_value = [&](const Bytes& /* value */) { return true; };
  ASSERT_THAT(shard->removeValues(k1, is_any_value), Eq(3));
  auto iter = shard->get(k1);
  ASSERT_FALSE(iter.hasNext());

  shard->put(k2, v1);
  shard->put(k2, v2);
  shard->put(k2, v3);
  auto is_v2_or_v3 =
      [&](const Bytes& value) { return value == v2 || value == v3; };
  ASSERT_TRUE(shard->removeValues(k2, is_v2_or_v3));
  iter = shard->get(k2);
  ASSERT_THAT(iter.next(), Eq(v1));
  ASSERT_FALSE(iter.hasNext());
}

TEST_F(ShardTestFixture, ReplaceValueReplacesOnlyFirstMatch) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  shard->put(k1, v2);
  shard->put(k1, v3);
  auto rotate_any = [&](const Bytes& value) {
    if (value == v1) return v2;
    if (value == v2) return v3;
    if (value == v3) return v1;
    return std::string();
  };
  ASSERT_TRUE(shard->replaceValue(k1, rotate_any));
  auto iter = shard->get(k1);
  ASSERT_THAT(iter.next(), Eq(v2));
  ASSERT_THAT(iter.next(), Eq(v3));
  ASSERT_THAT(iter.next(), Eq(v2));  // v1 replacement
  ASSERT_FALSE(iter.hasNext());

  shard->put(k2, v1);
  shard->put(k2, v2);
  shard->put(k2, v3);
  auto rotate_v2_or_v3 = [&](const Bytes& value) {
    if (value == v2) return v3;
    if (value == v3) return v1;
    return std::string();
  };
  ASSERT_TRUE(shard->replaceValue(k2, rotate_v2_or_v3));
  iter = shard->get(k2);
  ASSERT_THAT(iter.next(), Eq(v1));
  ASSERT_THAT(iter.next(), Eq(v3));
  ASSERT_THAT(iter.next(), Eq(v3));  // v2 replacement
  ASSERT_FALSE(iter.hasNext());
}

TEST_F(ShardTestFixture, ReplaceValuesReplacesAllMatches) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  shard->put(k1, v2);
  shard->put(k1, v3);
  auto rotate_any = [&](const Bytes& value) {
    if (value == v1) return v2;
    if (value == v2) return v3;
    if (value == v3) return v1;
    return std::string();
  };
  ASSERT_THAT(shard->replaceValues(k1, rotate_any), Eq(3));
  auto iter = shard->get(k1);
  ASSERT_THAT(iter.next(), Eq(v2));  // v1 replacement
  ASSERT_THAT(iter.next(), Eq(v3));  // v2 replacement
  ASSERT_THAT(iter.next(), Eq(v1));  // v3 replacement
  ASSERT_FALSE(iter.hasNext());

  shard->put(k2, v1);
  shard->put(k2, v2);
  shard->put(k2, v3);
  auto rotate_v2_or_v3 = [&](const Bytes& value) {
    if (value == v2) return v3;
    if (value == v3) return v1;
    return std::string();
  };
  ASSERT_THAT(shard->replaceValues(k2, rotate_v2_or_v3), Eq(2));
  iter = shard->get(k2);
  ASSERT_THAT(iter.next(), Eq(v1));
  ASSERT_THAT(iter.next(), Eq(v3));  // v2 replacement
  ASSERT_THAT(iter.next(), Eq(v1));  // v3 replacement
  ASSERT_FALSE(iter.hasNext());
}

TEST_F(ShardTestFixture, ForEachKeyIgnoresEmptyLists) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  shard->put(k2, v1);
  shard->put(k3, v1);
  std::vector<std::string> keys;
  auto collect = [&](const Bytes& key) { keys.push_back(key.toString()); };
  shard->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k1, k2, k3));

  keys.clear();
  shard->removeKey(k1);
  shard->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k2, k3));

  keys.clear();
  shard->removeKey(k2);
  shard->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k3));

  keys.clear();
  shard->removeKey(k3);
  shard->forEachKey(collect);
  ASSERT_TRUE(keys.empty());

  shard->put(k1, v1);
  shard->put(k2, v1);
  shard->put(k3, v1);
  shard->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k1, k2, k3));

  auto any_value = [](const Bytes& /* value */) { return true; };

  keys.clear();
  shard->removeValues(k1, any_value);
  shard->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k2, k3));

  keys.clear();
  shard->removeValues(k2, any_value);
  shard->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k3));

  keys.clear();
  shard->removeValues(k3, any_value);
  shard->forEachKey(collect);
  ASSERT_TRUE(keys.empty());
}

TEST_F(ShardTestFixture, ForEachValueVisitsAllValues) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  shard->put(k2, v1);
  shard->put(k2, v2);
  std::vector<std::string> values;
  auto collect = [&](const Bytes& val) { values.push_back(val.toString()); };
  shard->forEachValue(k1, collect);
  ASSERT_THAT(values, UnorderedElementsAre(v1));

  values.clear();
  shard->forEachValue(k2, collect);
  ASSERT_THAT(values, UnorderedElementsAre(v1, v2));

  values.clear();
  shard->forEachValue(k3, collect);
  ASSERT_TRUE(values.empty());
}

TEST_F(ShardTestFixture, ForEachEntryIgnoresEmptyLists) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  shard->put(k2, v1);
  shard->put(k2, v2);
  shard->put(k3, v1);
  shard->put(k3, v2);
  shard->put(k3, v3);
  std::map<std::string, std::vector<std::string>> mapping;
  auto collect = [&](const Bytes& key, SharedListIterator&& iter) {
    while (iter.hasNext()) {
      mapping[key.toString()].push_back(iter.next().toString());
    }
  };
  shard->forEachEntry(collect);
  ASSERT_THAT(mapping.size(), Eq(3));
  ASSERT_THAT(mapping.at(k1), ElementsAre(v1));
  ASSERT_THAT(mapping.at(k2), ElementsAre(v1, v2));
  ASSERT_THAT(mapping.at(k3), ElementsAre(v1, v2, v3));

  mapping.clear();
  shard->removeValues(k2, [](const Bytes& /* value */) { return true; });
  shard->forEachEntry(collect);
  ASSERT_THAT(mapping.size(), Eq(2));
  ASSERT_THAT(mapping.at(k1), ElementsAre(v1));
  ASSERT_THAT(mapping.at(k3), ElementsAre(v1, v2, v3));
}

TEST_F(ShardTestFixture, GetStatsReturnsCorrectValues) {
  auto shard = openOrCreateShard(prefix);
  shard->put("k", "vvvvv");
  shard->put("kk", "vvvv");
  shard->put("kk", "vvvv");
  shard->put("kkk", "vvv");
  shard->put("kkk", "vvv");
  shard->put("kkk", "vvv");
  shard->put("kkkk", "vv");
  shard->put("kkkk", "vv");
  shard->put("kkkk", "vv");
  shard->put("kkkk", "vv");
  shard->put("kkkkk", "v");
  shard->put("kkkkk", "v");
  shard->put("kkkkk", "v");
  shard->put("kkkkk", "v");
  shard->put("kkkkk", "v");

  auto stats = shard->getStats();
  ASSERT_THAT(stats.block_size, Eq(Shard::Options().block_size));
  ASSERT_THAT(stats.key_size_avg, Eq(3));
  ASSERT_THAT(stats.key_size_max, Eq(5));
  ASSERT_THAT(stats.key_size_min, Eq(1));
  ASSERT_THAT(stats.list_size_avg, Eq(3));
  ASSERT_THAT(stats.list_size_max, Eq(5));
  ASSERT_THAT(stats.list_size_min, Eq(1));
  ASSERT_THAT(stats.num_blocks, Eq(0));
  ASSERT_THAT(stats.num_keys_total, Eq(5));
  ASSERT_THAT(stats.num_keys_valid, Eq(5));
  ASSERT_THAT(stats.num_values_total, Eq(15));
  ASSERT_THAT(stats.num_values_valid, Eq(15));

  shard->removeKey("kkkkk");
  stats = shard->getStats();
  ASSERT_THAT(stats.block_size, Eq(Shard::Options().block_size));
  ASSERT_THAT(stats.key_size_avg, Eq(2));
  ASSERT_THAT(stats.key_size_max, Eq(4));
  ASSERT_THAT(stats.key_size_min, Eq(1));
  ASSERT_THAT(stats.list_size_avg, Eq(2));
  ASSERT_THAT(stats.list_size_max, Eq(4));
  ASSERT_THAT(stats.list_size_min, Eq(1));
  ASSERT_THAT(stats.num_blocks, Eq(0));
  ASSERT_THAT(stats.num_keys_total, Eq(5));
  ASSERT_THAT(stats.num_keys_valid, Eq(4));
  ASSERT_THAT(stats.num_values_total, Eq(15));
  ASSERT_THAT(stats.num_values_valid, Eq(10));
}

TEST_F(ShardTestFixture, GetStatsReturnsCorrectValuesAfterRemovingKeys) {
  {
    auto shard = openOrCreateShard(prefix);
    shard->put("k", "vvvvv");
    shard->put("kk", "vvvv");
    shard->put("kk", "vvvv");
    shard->put("kkk", "vvv");
    shard->put("kkk", "vvv");
    shard->put("kkk", "vvv");
    shard->put("kkkk", "vv");
    shard->put("kkkk", "vv");
    shard->put("kkkk", "vv");
    shard->put("kkkk", "vv");
    shard->put("kkkkk", "v");
    shard->put("kkkkk", "v");
    shard->put("kkkkk", "v");
    shard->put("kkkkk", "v");
    shard->put("kkkkk", "v");

    shard->removeKey("k");
    shard->removeKeys([](const Bytes& key) { return key == "kk"; });

    const auto stats = shard->getStats();
    ASSERT_THAT(stats.num_keys_total, Eq(5));
    ASSERT_THAT(stats.num_keys_valid, Eq(3));
    ASSERT_THAT(stats.num_values_total, Eq(15));
    ASSERT_THAT(stats.num_values_valid, Eq(12));
  }

  auto shard = openOrCreateShard(prefix);
  const auto stats = shard->getStats();
  ASSERT_THAT(stats.num_keys_total, Eq(3));
  ASSERT_THAT(stats.num_keys_valid, Eq(3));
  ASSERT_THAT(stats.num_values_total, Eq(15));
  ASSERT_THAT(stats.num_values_valid, Eq(12));
}

TEST_F(ShardTestFixture, GetStatsReturnsCorrectValuesAfterRemovingValues) {
  {
    auto shard = openOrCreateShard(prefix);
    shard->put("k", "vvvvv");
    shard->put("kk", "vvvv");
    shard->put("kk", "vvvv");
    shard->put("kkk", "vvv");
    shard->put("kkk", "vvv");
    shard->put("kkk", "vvv");
    shard->put("kkkk", "vv");
    shard->put("kkkk", "vv");
    shard->put("kkkk", "vv");
    shard->put("kkkk", "vv");
    shard->put("kkkkk", "v");
    shard->put("kkkkk", "v");
    shard->put("kkkkk", "v");
    shard->put("kkkkk", "v");
    shard->put("kkkkk", "v");

    shard->removeValue("k", Equal("vvvvv"));
    shard->removeValues("kk", [](const Bytes& val) { return val == "vvvv"; });

    const auto stats = shard->getStats();
    ASSERT_THAT(stats.num_keys_total, Eq(5));
    ASSERT_THAT(stats.num_keys_valid, Eq(3));
    ASSERT_THAT(stats.num_values_total, Eq(15));
    ASSERT_THAT(stats.num_values_valid, Eq(12));
  }

  auto shard = openOrCreateShard(prefix);
  const auto stats = shard->getStats();
  ASSERT_THAT(stats.num_keys_total, Eq(3));
  ASSERT_THAT(stats.num_keys_valid, Eq(3));
  ASSERT_THAT(stats.num_values_total, Eq(15));
  ASSERT_THAT(stats.num_values_valid, Eq(12));
}

TEST_F(ShardTestFixture, IsReadOnlyReturnsCorrectValue) {
  {
    auto shard = openOrCreateShard(prefix);
    ASSERT_FALSE(shard->isReadOnly());
  }
  {
    auto shard = openOrCreateShardAsReadOnly(prefix);
    ASSERT_TRUE(shard->isReadOnly());
  }
}

TEST_F(ShardTestFixture, PutThrowsIfOpenedAsReadOnly) {
  auto shard = openOrCreateShardAsReadOnly(prefix);
  ASSERT_THROW(shard->put(k1, v1), mt::AssertionError);
}

TEST_F(ShardTestFixture, RemoveKeyThrowsIfOpenedAsReadOnly) {
  auto shard = openOrCreateShardAsReadOnly(prefix);
  ASSERT_THROW(shard->removeKey(k1), mt::AssertionError);
}

TEST_F(ShardTestFixture, RemoveKeysThrowsIfOpenedAsReadOnly) {
  auto shard = openOrCreateShardAsReadOnly(prefix);
  ASSERT_THROW(shard->removeKeys([](const Bytes& /* key */) { return true; }),
               mt::AssertionError);
}

TEST_F(ShardTestFixture, RemoveValueThrowsIfOpenedAsReadOnly) {
  auto shard = openOrCreateShardAsReadOnly(prefix);
  ASSERT_THROW(shard->removeValue(k1, Equal(v1)), mt::AssertionError);
}

TEST_F(ShardTestFixture, RemoveValuesThrowsIfOpenedAsReadOnly) {
  auto shard = openOrCreateShardAsReadOnly(prefix);
  ASSERT_THROW(shard->removeValues(k1, Equal(v1)), mt::AssertionError);
}

TEST_F(ShardTestFixture, ReplaceValueThrowsIfOpenedAsReadOnly) {
  auto shard = openOrCreateShardAsReadOnly(prefix);
  ASSERT_THROW(shard->replaceValue(k1, v1, v2), mt::AssertionError);
}

TEST_F(ShardTestFixture, ReplaceValuesThrowsIfOpenedAsReadOnly) {
  auto shard = openOrCreateShardAsReadOnly(prefix);
  ASSERT_THROW(shard->replaceValues(k1, v1, v2), mt::AssertionError);
}

TEST_F(ShardTestFixture, GetBlockSizeReturnsCorrectValue) {
  auto shard = openOrCreateShard(prefix);
  ASSERT_THAT(shard->getBlockSize(), Eq(Shard::Options().block_size));
}

// -----------------------------------------------------------------------------
// class Shard / Mutability
// -----------------------------------------------------------------------------

struct ShardTestWithParam : public testing::TestWithParam<int> {
  void SetUp() override {
    boost::filesystem::remove_all(directory);
    boost::filesystem::create_directory(directory);
  }

  void TearDown() override { boost::filesystem::remove_all(directory); }

  const boost::filesystem::path directory = "/tmp/multimap.ShardTestWithParam";
  const boost::filesystem::path prefix = directory / "shard";
};

TEST_P(ShardTestWithParam, PutDataThenReadAll) {
  {
    auto shard = openOrCreateShard(prefix);
    for (auto k = 0; k != GetParam(); ++k) {
      for (auto v = 0; v != GetParam(); ++v) {
        shard->put(std::to_string(k), std::to_string(v));
      }
    }

    for (auto k = 0; k != GetParam(); ++k) {
      auto iter = shard->get(std::to_string(k));
      for (auto v = 0; v != GetParam(); ++v) {
        ASSERT_TRUE(iter.hasNext());
        ASSERT_THAT(iter.next(), std::to_string(v));
      }
      ASSERT_FALSE(iter.hasNext());
    }
  }

  auto shard = openOrCreateShard(prefix);
  for (auto k = 0; k != GetParam(); ++k) {
    auto iter = shard->get(std::to_string(k));
    for (auto v = 0; v != GetParam(); ++v) {
      ASSERT_TRUE(iter.hasNext());
      ASSERT_THAT(iter.next(), std::to_string(v));
    }
    ASSERT_FALSE(iter.hasNext());
  }
}

TEST_P(ShardTestWithParam, PutDataThenRemoveSomeThenReadAll) {
  auto is_odd = [](const Bytes& b) { return std::stoul(b.toString()) % 2; };
  {
    auto shard = openOrCreateShard(prefix);
    for (auto k = 0; k != GetParam(); ++k) {
      for (auto v = 0; v != GetParam(); ++v) {
        shard->put(std::to_string(k), std::to_string(v));
      }
    }

    for (auto k = 0; k != GetParam(); ++k) {
      const auto key = std::to_string(k);
      if (is_odd(key)) {
        shard->removeValues(key, is_odd);
      }
    }

    for (auto k = 0; k != GetParam(); ++k) {
      const auto key = std::to_string(k);
      auto iter = shard->get(key);
      if (is_odd(key)) {
        for (auto v = 0; v != GetParam(); ++v) {
          const auto value = std::to_string(v);
          if (is_odd(value)) {
            // This value must have been removed.
          } else {
            ASSERT_TRUE(iter.hasNext());
            ASSERT_THAT(iter.next(), Eq(value));
          }
        }
        ASSERT_FALSE(iter.hasNext());
      } else {
        for (auto v = 0; v != GetParam(); ++v) {
          ASSERT_TRUE(iter.hasNext());
          ASSERT_THAT(iter.next(), Eq(std::to_string(v)));
        }
        ASSERT_FALSE(iter.hasNext());
      }
    }
  }

  auto shard = openOrCreateShard(prefix);
  for (auto k = 0; k != GetParam(); ++k) {
    const auto key = std::to_string(k);
    auto iter = shard->get(key);
    if (is_odd(key)) {
      for (auto v = 0; v != GetParam(); ++v) {
        const auto value = std::to_string(v);
        if (is_odd(value)) {
          // This value must have been removed.
        } else {
          ASSERT_TRUE(iter.hasNext());
          ASSERT_THAT(iter.next(), Eq(value));
        }
      }
      ASSERT_FALSE(iter.hasNext());
    } else {
      for (auto v = 0; v != GetParam(); ++v) {
        ASSERT_TRUE(iter.hasNext());
        ASSERT_THAT(iter.next(), Eq(std::to_string(v)));
      }
      ASSERT_FALSE(iter.hasNext());
    }
  }
}

INSTANTIATE_TEST_CASE_P(Parameterized, ShardTestWithParam,
                        testing::Values(0, 1, 2, 10, 100, 1000));

// INSTANTIATE_TEST_CASE_P(ParameterizedLongRunning, ShardTestWithParam,
//                         testing::Values(10000, 100000));

// -----------------------------------------------------------------------------
// class Shard / Concurrency
// -----------------------------------------------------------------------------

const auto NULL_PROCEDURE = [](const Bytes&) {};
const auto TRUE_PREDICATE = [](const Bytes&) { return true; };
const auto EMPTY_FUNCTION = [](const Bytes&) { return std::string(); };

TEST_F(ShardTestFixture, GetDifferentListsDoesNotBlock) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  shard->put(k2, v1);
  auto iter1 = shard->get(k1);
  ASSERT_TRUE(iter1.hasNext());
  auto iter2 = shard->get(k2);
  ASSERT_TRUE(iter2.hasNext());
}

TEST_F(ShardTestFixture, GetSameListTwiceDoesNotBlock) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  auto iter1 = shard->get(k1);
  ASSERT_TRUE(iter1.hasNext());
  auto iter2 = shard->get(k1);
  ASSERT_TRUE(iter2.hasNext());
}

TEST_F(ShardTestFixture, RemoveKeyBlocksIfListIsLocked) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  auto thread_is_running = true;
  {
    auto iter = shard->get(k1);
    ASSERT_TRUE(iter.hasNext());
    std::thread thread([&] {
      shard->removeKey(k1);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(ShardTestFixture, RemoveKeysBlocksIfListIsLocked) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  auto thread_is_running = true;
  {
    auto iter = shard->get(k1);
    ASSERT_TRUE(iter.hasNext());
    std::thread thread([&] {
      shard->removeKeys(TRUE_PREDICATE);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(ShardTestFixture, RemoveValueBlocksIfListIsLocked) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  auto thread_is_running = true;
  {
    auto iter = shard->get(k1);
    ASSERT_TRUE(iter.hasNext());
    std::thread thread([&] {
      shard->removeValue(k1, TRUE_PREDICATE);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(ShardTestFixture, RemoveValuesBlocksIfListIsLocked) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  auto thread_is_running = true;
  {
    auto iter = shard->get(k1);
    ASSERT_TRUE(iter.hasNext());
    std::thread thread([&] {
      shard->removeValues(k1, TRUE_PREDICATE);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(ShardTestFixture, ReplaceValueBlocksIfListIsLocked) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  auto thread_is_running = true;
  {
    auto iter = shard->get(k1);
    ASSERT_TRUE(iter.hasNext());
    std::thread thread([&] {
      shard->replaceValue(k1, EMPTY_FUNCTION);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(ShardTestFixture, ReplaceValuesBlocksIfListIsLocked) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  auto thread_is_running = true;
  {
    auto iter = shard->get(k1);
    ASSERT_TRUE(iter.hasNext());
    std::thread thread([&] {
      shard->replaceValues(k1, EMPTY_FUNCTION);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(ShardTestFixture, ForEachKeyDoesNotBlockIfListIsLocked) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  auto thread_is_running = true;
  std::thread thread([&] {
    shard->forEachKey(NULL_PROCEDURE);
    thread_is_running = false;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(ShardTestFixture, ForEachValueDoesNotBlockIfListIsLocked) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  auto thread_is_running = true;
  std::thread thread([&] {
    shard->forEachValue(k1, NULL_PROCEDURE);
    thread_is_running = false;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(ShardTestFixture, ForEachEntryDoesNotBlockIfListIsLocked) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  auto thread_is_running = true;
  std::thread thread([&] {
    shard->forEachEntry([](const Bytes&, Shard::Iterator&&) {});
    thread_is_running = false;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

// -----------------------------------------------------------------------------
// class Shard::Stats
// -----------------------------------------------------------------------------

TEST(ShardStatsTest, NamesAndToVectorHaveSameDimension) {
  const auto names = Shard::Stats::names();
  const auto vector = Shard::Stats().toVector();
  ASSERT_THAT(names.size(), Eq(vector.size()));
}

}  // namespace internal
}  // namespace multimap
