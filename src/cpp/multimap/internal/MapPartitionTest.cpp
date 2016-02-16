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

#include <thread>
#include <type_traits>
#include <boost/filesystem/operations.hpp>
#include "gmock/gmock.h"
#include "multimap/internal/MapPartition.hpp"
#include "multimap/callables.hpp"

namespace multimap {
namespace internal {

using testing::Eq;
using testing::ElementsAre;
using testing::UnorderedElementsAre;

std::unique_ptr<MapPartition> openMapPartition(
    const boost::filesystem::path& prefix,
    const MapPartition::Options& options) {
  return std::unique_ptr<MapPartition>(new MapPartition(prefix, options));
}

std::unique_ptr<MapPartition> openOrCreateMapPartition(
    const boost::filesystem::path& prefix) {
  MapPartition::Options options;
  options.create_if_missing = true;
  return std::unique_ptr<MapPartition>(new MapPartition(prefix, options));
}

std::unique_ptr<MapPartition> openOrCreateMapPartitionAsReadOnly(
    const boost::filesystem::path& prefix) {
  MapPartition::Options options;
  options.readonly = true;
  options.create_if_missing = true;
  return std::unique_ptr<MapPartition>(new MapPartition(prefix, options));
}

TEST(MapPartitionTest, IsNotDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<MapPartition>::value);
}

TEST(MapPartitionTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<MapPartition>::value);
  ASSERT_FALSE(std::is_copy_assignable<MapPartition>::value);
}

TEST(MapPartitionTest, IsNotMoveConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_move_constructible<MapPartition>::value);
  ASSERT_FALSE(std::is_move_assignable<MapPartition>::value);
}

struct MapPartitionTestFixture : public testing::Test {
  void SetUp() override {
    boost::filesystem::remove_all(directory);
    boost::filesystem::create_directory(directory);
  }

  void TearDown() override { boost::filesystem::remove_all(directory); }

  const boost::filesystem::path directory =
      "/tmp/multimap.MapPartitionTestFixture";
  const boost::filesystem::path prefix = directory / "partition";
  const std::string k1 = "k1";
  const std::string k2 = "k2";
  const std::string k3 = "k3";
  const std::string v1 = "v1";
  const std::string v2 = "v2";
  const std::string v3 = "v3";
};

TEST_F(MapPartitionTestFixture, ConstructorThrowsIfNotExist) {
  ASSERT_THROW(MapPartition(this->prefix), std::runtime_error);
  // GCC complains when not using 'this' pointer.
}

TEST_F(MapPartitionTestFixture, ConstructorWithDefaultOptionsThrowsIfNotExist) {
  MapPartition::Options options;
  ASSERT_THROW(MapPartition(prefix, options), std::runtime_error);
}

TEST_F(MapPartitionTestFixture, ConstructorWithCreateIfMissingDoesNotThrow) {
  MapPartition::Options options;
  options.create_if_missing = true;
  ASSERT_NO_THROW(MapPartition(prefix, options));
}

TEST_F(MapPartitionTestFixture, PutAppendsValuesToList) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  partition->put(k1, v2);
  partition->put(k1, v3);
  auto iter = partition->get(k1);
  ASSERT_THAT(iter->next(), Eq(v1));
  ASSERT_THAT(iter->next(), Eq(v2));
  ASSERT_THAT(iter->next(), Eq(v3));
  ASSERT_FALSE(iter->hasNext());
}

TEST_F(MapPartitionTestFixture, PutMaxKeyDoesNotThrow) {
  auto partition = openOrCreateMapPartition(prefix);
  try {
    std::string key(MapPartition::Limits::maxKeySize(), 'k');
    ASSERT_NO_THROW(partition->put(key, v1));
  } catch (...) {
    // Allocating key itself may fail.
  }
}

TEST_F(MapPartitionTestFixture, PutTooBigKeyThrows) {
  auto partition = openOrCreateMapPartition(prefix);
  try {
    std::string key(MapPartition::Limits::maxKeySize() + 1, 'k');
    ASSERT_THROW(partition->put(key, v1), mt::AssertionError);
  } catch (...) {
    // Allocating key itself may fail.
  }
}

TEST_F(MapPartitionTestFixture, PutMaxValueDoesNotThrow) {
  auto partition = openOrCreateMapPartition(prefix);
  try {
    std::string value(MapPartition::Limits::maxValueSize(), 'v');
    ASSERT_NO_THROW(partition->put(k1, value));
  } catch (...) {
    // Allocating value itself may fail.
  }
}

TEST_F(MapPartitionTestFixture, PutTooBigValueThrows) {
  auto partition = openOrCreateMapPartition(prefix);
  try {
    std::string value(MapPartition::Limits::maxValueSize() + 1, 'v');
    ASSERT_THROW(partition->put(k1, value), mt::AssertionError);
  } catch (...) {
    // Allocating value itself may fail.
  }
}

TEST_F(MapPartitionTestFixture, PutValuesAndReopenInBetween) {
  {
    MapPartition::Options options;
    options.block_size = 128;
    options.create_if_missing = true;
    auto partition = openMapPartition(prefix, options);
    partition->put(k1, v1);
    partition->put(k2, v1);
    partition->put(k3, v1);
  }
  {
    auto partition = openOrCreateMapPartition(prefix);
    partition->put(k1, v2);
    partition->put(k2, v2);
    partition->put(k3, v2);
  }
  {
    auto partition = openOrCreateMapPartition(prefix);
    partition->put(k1, v3);
    partition->put(k2, v3);
    partition->put(k3, v3);
  }
  auto partition = openOrCreateMapPartition(prefix);
  auto iter1 = partition->get(k1);
  ASSERT_THAT(iter1->next(), Eq(v1));
  ASSERT_THAT(iter1->next(), Eq(v2));
  ASSERT_THAT(iter1->next(), Eq(v3));
  ASSERT_FALSE(iter1->hasNext());

  auto iter2 = partition->get(k2);
  ASSERT_THAT(iter2->next(), Eq(v1));
  ASSERT_THAT(iter2->next(), Eq(v2));
  ASSERT_THAT(iter2->next(), Eq(v3));
  ASSERT_FALSE(iter2->hasNext());

  auto iter3 = partition->get(k3);
  ASSERT_THAT(iter3->next(), Eq(v1));
  ASSERT_THAT(iter3->next(), Eq(v2));
  ASSERT_THAT(iter3->next(), Eq(v3));
  ASSERT_FALSE(iter3->hasNext());
}

TEST_F(MapPartitionTestFixture, GetReturnsEmptyListForNonExistingKey) {
  auto partition = openOrCreateMapPartition(prefix);
  ASSERT_FALSE(partition->get(k1)->hasNext());
}

TEST_F(MapPartitionTestFixture, RemoveKeyRemovesMappedValues) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  partition->put(k2, v1);
  partition->put(k3, v1);
  ASSERT_TRUE(partition->removeKey(k1));
  ASSERT_TRUE(partition->removeKey(k2));
  ASSERT_FALSE(partition->get(k1)->hasNext());
  ASSERT_FALSE(partition->get(k2)->hasNext());
  ASSERT_TRUE(partition->get(k3)->hasNext());
}

TEST_F(MapPartitionTestFixture, RemoveKeysRemovesMappedValues) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  partition->put(k2, v1);
  partition->put(k3, v1);
  auto is_k1_or_k2 = [&](const Bytes& key) { return key == k1 || key == k2; };
  ASSERT_THAT(partition->removeKeys(is_k1_or_k2), Eq(2));
  ASSERT_FALSE(partition->get(k1)->hasNext());
  ASSERT_FALSE(partition->get(k2)->hasNext());
  ASSERT_TRUE(partition->get(k3)->hasNext());
}

TEST_F(MapPartitionTestFixture, RemoveValueRemovesOnlyFirstMatch) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  partition->put(k1, v2);
  partition->put(k1, v3);
  auto is_any_value = [&](const Bytes& /* value */) { return true; };
  ASSERT_TRUE(partition->removeValue(k1, is_any_value));
  auto iter = partition->get(k1);
  ASSERT_THAT(iter->next(), Eq(v2));
  ASSERT_THAT(iter->next(), Eq(v3));
  ASSERT_FALSE(iter->hasNext());

  partition->put(k2, v1);
  partition->put(k2, v2);
  partition->put(k2, v3);
  auto is_v2_or_v3 =
      [&](const Bytes& value) { return value == v2 || value == v3; };
  ASSERT_TRUE(partition->removeValue(k2, is_v2_or_v3));
  iter = partition->get(k2);
  ASSERT_THAT(iter->next(), Eq(v1));
  ASSERT_THAT(iter->next(), Eq(v3));
  ASSERT_FALSE(iter->hasNext());
}

TEST_F(MapPartitionTestFixture, RemoveValuesRemovesAllMatches) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  partition->put(k1, v2);
  partition->put(k1, v3);
  auto is_any_value = [&](const Bytes& /* value */) { return true; };
  ASSERT_THAT(partition->removeValues(k1, is_any_value), Eq(3));
  auto iter = partition->get(k1);
  ASSERT_FALSE(iter->hasNext());

  partition->put(k2, v1);
  partition->put(k2, v2);
  partition->put(k2, v3);
  auto is_v2_or_v3 =
      [&](const Bytes& value) { return value == v2 || value == v3; };
  ASSERT_TRUE(partition->removeValues(k2, is_v2_or_v3));
  iter = partition->get(k2);
  ASSERT_THAT(iter->next(), Eq(v1));
  ASSERT_FALSE(iter->hasNext());
}

TEST_F(MapPartitionTestFixture, ReplaceValueReplacesOnlyFirstMatch) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  partition->put(k1, v2);
  partition->put(k1, v3);
  auto rotate_any = [&](const Bytes& value) {
    if (value == v1) return v2;
    if (value == v2) return v3;
    if (value == v3) return v1;
    return std::string();
  };
  ASSERT_TRUE(partition->replaceValue(k1, rotate_any));
  auto iter = partition->get(k1);
  ASSERT_THAT(iter->next(), Eq(v2));
  ASSERT_THAT(iter->next(), Eq(v3));
  ASSERT_THAT(iter->next(), Eq(v2));  // v1 replacement
  ASSERT_FALSE(iter->hasNext());

  partition->put(k2, v1);
  partition->put(k2, v2);
  partition->put(k2, v3);
  auto rotate_v2_or_v3 = [&](const Bytes& value) {
    if (value == v2) return v3;
    if (value == v3) return v1;
    return std::string();
  };
  ASSERT_TRUE(partition->replaceValue(k2, rotate_v2_or_v3));
  iter = partition->get(k2);
  ASSERT_THAT(iter->next(), Eq(v1));
  ASSERT_THAT(iter->next(), Eq(v3));
  ASSERT_THAT(iter->next(), Eq(v3));  // v2 replacement
  ASSERT_FALSE(iter->hasNext());
}

TEST_F(MapPartitionTestFixture, ReplaceValuesReplacesAllMatches) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  partition->put(k1, v2);
  partition->put(k1, v3);
  auto rotate_any = [&](const Bytes& value) {
    if (value == v1) return v2;
    if (value == v2) return v3;
    if (value == v3) return v1;
    return std::string();
  };
  ASSERT_THAT(partition->replaceValues(k1, rotate_any), Eq(3));
  auto iter = partition->get(k1);
  ASSERT_THAT(iter->next(), Eq(v2));  // v1 replacement
  ASSERT_THAT(iter->next(), Eq(v3));  // v2 replacement
  ASSERT_THAT(iter->next(), Eq(v1));  // v3 replacement
  ASSERT_FALSE(iter->hasNext());

  partition->put(k2, v1);
  partition->put(k2, v2);
  partition->put(k2, v3);
  auto rotate_v2_or_v3 = [&](const Bytes& value) {
    if (value == v2) return v3;
    if (value == v3) return v1;
    return std::string();
  };
  ASSERT_THAT(partition->replaceValues(k2, rotate_v2_or_v3), Eq(2));
  iter = partition->get(k2);
  ASSERT_THAT(iter->next(), Eq(v1));
  ASSERT_THAT(iter->next(), Eq(v3));  // v2 replacement
  ASSERT_THAT(iter->next(), Eq(v1));  // v3 replacement
  ASSERT_FALSE(iter->hasNext());
}

TEST_F(MapPartitionTestFixture, ForEachKeyIgnoresEmptyLists) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  partition->put(k2, v1);
  partition->put(k3, v1);
  std::vector<std::string> keys;
  auto collect = [&](const Bytes& key) { keys.push_back(key.toString()); };
  partition->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k1, k2, k3));

  keys.clear();
  partition->removeKey(k1);
  partition->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k2, k3));

  keys.clear();
  partition->removeKey(k2);
  partition->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k3));

  keys.clear();
  partition->removeKey(k3);
  partition->forEachKey(collect);
  ASSERT_TRUE(keys.empty());

  partition->put(k1, v1);
  partition->put(k2, v1);
  partition->put(k3, v1);
  partition->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k1, k2, k3));

  auto any_value = [](const Bytes& /* value */) { return true; };

  keys.clear();
  partition->removeValues(k1, any_value);
  partition->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k2, k3));

  keys.clear();
  partition->removeValues(k2, any_value);
  partition->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k3));

  keys.clear();
  partition->removeValues(k3, any_value);
  partition->forEachKey(collect);
  ASSERT_TRUE(keys.empty());
}

TEST_F(MapPartitionTestFixture, ForEachValueVisitsAllValues) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  partition->put(k2, v1);
  partition->put(k2, v2);
  std::vector<std::string> values;
  auto collect = [&](const Bytes& val) { values.push_back(val.toString()); };
  partition->forEachValue(k1, collect);
  ASSERT_THAT(values, UnorderedElementsAre(v1));

  values.clear();
  partition->forEachValue(k2, collect);
  ASSERT_THAT(values, UnorderedElementsAre(v1, v2));

  values.clear();
  partition->forEachValue(k3, collect);
  ASSERT_TRUE(values.empty());
}

TEST_F(MapPartitionTestFixture, ForEachEntryIgnoresEmptyLists) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  partition->put(k2, v1);
  partition->put(k2, v2);
  partition->put(k3, v1);
  partition->put(k3, v2);
  partition->put(k3, v3);
  std::map<std::string, std::vector<std::string> > mapping;
  auto collect = [&](const Bytes& key, Iterator* iter) {
    while (iter->hasNext()) {
      mapping[key.toString()].push_back(iter->next().toString());
    }
  };
  partition->forEachEntry(collect);
  ASSERT_THAT(mapping.size(), Eq(3));
  ASSERT_THAT(mapping.at(k1), ElementsAre(v1));
  ASSERT_THAT(mapping.at(k2), ElementsAre(v1, v2));
  ASSERT_THAT(mapping.at(k3), ElementsAre(v1, v2, v3));

  mapping.clear();
  partition->removeValues(k2, [](const Bytes& /* value */) { return true; });
  partition->forEachEntry(collect);
  ASSERT_THAT(mapping.size(), Eq(2));
  ASSERT_THAT(mapping.at(k1), ElementsAre(v1));
  ASSERT_THAT(mapping.at(k3), ElementsAre(v1, v2, v3));
}

TEST_F(MapPartitionTestFixture, GetStatsReturnsCorrectValues) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put("k", "vvvvv");
  partition->put("kk", "vvvv");
  partition->put("kk", "vvvv");
  partition->put("kkk", "vvv");
  partition->put("kkk", "vvv");
  partition->put("kkk", "vvv");
  partition->put("kkkk", "vv");
  partition->put("kkkk", "vv");
  partition->put("kkkk", "vv");
  partition->put("kkkk", "vv");
  partition->put("kkkkk", "v");
  partition->put("kkkkk", "v");
  partition->put("kkkkk", "v");
  partition->put("kkkkk", "v");
  partition->put("kkkkk", "v");

  auto stats = partition->getStats();
  ASSERT_THAT(stats.block_size, Eq(MapPartition::Options().block_size));
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

  partition->removeKey("kkkkk");
  stats = partition->getStats();
  ASSERT_THAT(stats.block_size, Eq(MapPartition::Options().block_size));
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

TEST_F(MapPartitionTestFixture, GetStatsReturnsCorrectValuesAfterRemovingKeys) {
  {
    auto partition = openOrCreateMapPartition(prefix);
    partition->put("k", "vvvvv");
    partition->put("kk", "vvvv");
    partition->put("kk", "vvvv");
    partition->put("kkk", "vvv");
    partition->put("kkk", "vvv");
    partition->put("kkk", "vvv");
    partition->put("kkkk", "vv");
    partition->put("kkkk", "vv");
    partition->put("kkkk", "vv");
    partition->put("kkkk", "vv");
    partition->put("kkkkk", "v");
    partition->put("kkkkk", "v");
    partition->put("kkkkk", "v");
    partition->put("kkkkk", "v");
    partition->put("kkkkk", "v");

    partition->removeKey("k");
    partition->removeKeys([](const Bytes& key) { return key == "kk"; });

    const auto stats = partition->getStats();
    ASSERT_THAT(stats.num_keys_total, Eq(5));
    ASSERT_THAT(stats.num_keys_valid, Eq(3));
    ASSERT_THAT(stats.num_values_total, Eq(15));
    ASSERT_THAT(stats.num_values_valid, Eq(12));
  }

  auto partition = openOrCreateMapPartition(prefix);
  const auto stats = partition->getStats();
  ASSERT_THAT(stats.num_keys_total, Eq(3));
  ASSERT_THAT(stats.num_keys_valid, Eq(3));
  ASSERT_THAT(stats.num_values_total, Eq(15));
  ASSERT_THAT(stats.num_values_valid, Eq(12));
}

TEST_F(MapPartitionTestFixture,
       GetStatsReturnsCorrectValuesAfterRemovingValues) {
  {
    auto partition = openOrCreateMapPartition(prefix);
    partition->put("k", "vvvvv");
    partition->put("kk", "vvvv");
    partition->put("kk", "vvvv");
    partition->put("kkk", "vvv");
    partition->put("kkk", "vvv");
    partition->put("kkk", "vvv");
    partition->put("kkkk", "vv");
    partition->put("kkkk", "vv");
    partition->put("kkkk", "vv");
    partition->put("kkkk", "vv");
    partition->put("kkkkk", "v");
    partition->put("kkkkk", "v");
    partition->put("kkkkk", "v");
    partition->put("kkkkk", "v");
    partition->put("kkkkk", "v");

    partition->removeValue("k", Equal("vvvvv"));
    partition->removeValues("kk",
                            [](const Bytes& val) { return val == "vvvv"; });

    const auto stats = partition->getStats();
    ASSERT_THAT(stats.num_keys_total, Eq(5));
    ASSERT_THAT(stats.num_keys_valid, Eq(3));
    ASSERT_THAT(stats.num_values_total, Eq(15));
    ASSERT_THAT(stats.num_values_valid, Eq(12));
  }

  auto partition = openOrCreateMapPartition(prefix);
  const auto stats = partition->getStats();
  ASSERT_THAT(stats.num_keys_total, Eq(3));
  ASSERT_THAT(stats.num_keys_valid, Eq(3));
  ASSERT_THAT(stats.num_values_total, Eq(15));
  ASSERT_THAT(stats.num_values_valid, Eq(12));
}

TEST_F(MapPartitionTestFixture, IsReadOnlyReturnsCorrectValue) {
  {
    auto partition = openOrCreateMapPartition(prefix);
    ASSERT_FALSE(partition->isReadOnly());
  }
  {
    auto partition = openOrCreateMapPartitionAsReadOnly(prefix);
    ASSERT_TRUE(partition->isReadOnly());
  }
}

TEST_F(MapPartitionTestFixture, PutThrowsIfOpenedAsReadOnly) {
  auto partition = openOrCreateMapPartitionAsReadOnly(prefix);
  ASSERT_THROW(partition->put(k1, v1), std::runtime_error);
}

TEST_F(MapPartitionTestFixture, RemoveKeyThrowsIfOpenedAsReadOnly) {
  auto partition = openOrCreateMapPartitionAsReadOnly(prefix);
  ASSERT_THROW(partition->removeKey(k1), std::runtime_error);
}

TEST_F(MapPartitionTestFixture, RemoveKeysThrowsIfOpenedAsReadOnly) {
  auto partition = openOrCreateMapPartitionAsReadOnly(prefix);
  ASSERT_THROW(
      partition->removeKeys([](const Bytes& /* key */) { return true; }),
      std::runtime_error);
}

TEST_F(MapPartitionTestFixture, RemoveValueThrowsIfOpenedAsReadOnly) {
  auto partition = openOrCreateMapPartitionAsReadOnly(prefix);
  ASSERT_THROW(partition->removeValue(k1, Equal(v1)), std::runtime_error);
}

TEST_F(MapPartitionTestFixture, RemoveValuesThrowsIfOpenedAsReadOnly) {
  auto partition = openOrCreateMapPartitionAsReadOnly(prefix);
  ASSERT_THROW(partition->removeValues(k1, Equal(v1)), std::runtime_error);
}

TEST_F(MapPartitionTestFixture, ReplaceValueThrowsIfOpenedAsReadOnly) {
  auto partition = openOrCreateMapPartitionAsReadOnly(prefix);
  ASSERT_THROW(partition->replaceValue(k1, v1, v2), std::runtime_error);
}

TEST_F(MapPartitionTestFixture, ReplaceValuesThrowsIfOpenedAsReadOnly) {
  auto partition = openOrCreateMapPartitionAsReadOnly(prefix);
  ASSERT_THROW(partition->replaceValues(k1, v1, v2), std::runtime_error);
}

TEST_F(MapPartitionTestFixture, GetBlockSizeReturnsCorrectValue) {
  auto partition = openOrCreateMapPartition(prefix);
  ASSERT_THAT(partition->getBlockSize(),
              Eq(MapPartition::Options().block_size));
}

// -----------------------------------------------------------------------------
// class MapPartition / Mutability
// -----------------------------------------------------------------------------

struct MapPartitionTestWithParam : public testing::TestWithParam<int> {
  void SetUp() override {
    boost::filesystem::remove_all(directory);
    boost::filesystem::create_directory(directory);
  }

  void TearDown() override { boost::filesystem::remove_all(directory); }

  const boost::filesystem::path directory =
      "/tmp/multimap.MapPartitionTestWithParam";
  const boost::filesystem::path prefix = directory / "partition";
};

TEST_P(MapPartitionTestWithParam, PutDataThenReadAll) {
  {
    auto partition = openOrCreateMapPartition(prefix);
    for (auto k = 0; k != GetParam(); ++k) {
      for (auto v = 0; v != GetParam(); ++v) {
        partition->put(std::to_string(k), std::to_string(v));
      }
    }

    for (auto k = 0; k != GetParam(); ++k) {
      auto iter = partition->get(std::to_string(k));
      for (auto v = 0; v != GetParam(); ++v) {
        ASSERT_TRUE(iter->hasNext());
        ASSERT_THAT(iter->next(), std::to_string(v));
      }
      ASSERT_FALSE(iter->hasNext());
    }
  }

  auto partition = openOrCreateMapPartition(prefix);
  for (auto k = 0; k != GetParam(); ++k) {
    auto iter = partition->get(std::to_string(k));
    for (auto v = 0; v != GetParam(); ++v) {
      ASSERT_TRUE(iter->hasNext());
      ASSERT_THAT(iter->next(), std::to_string(v));
    }
    ASSERT_FALSE(iter->hasNext());
  }
}

TEST_P(MapPartitionTestWithParam, PutDataThenRemoveSomeThenReadAll) {
  auto is_odd = [](const Bytes& b) { return std::stoul(b.toString()) % 2; };
  {
    auto partition = openOrCreateMapPartition(prefix);
    for (auto k = 0; k != GetParam(); ++k) {
      for (auto v = 0; v != GetParam(); ++v) {
        partition->put(std::to_string(k), std::to_string(v));
      }
    }

    for (auto k = 0; k != GetParam(); ++k) {
      const auto key = std::to_string(k);
      if (is_odd(key)) {
        partition->removeValues(key, is_odd);
      }
    }

    for (auto k = 0; k != GetParam(); ++k) {
      const auto key = std::to_string(k);
      auto iter = partition->get(key);
      if (is_odd(key)) {
        for (auto v = 0; v != GetParam(); ++v) {
          const auto value = std::to_string(v);
          if (is_odd(value)) {
            // This value must have been removed.
          } else {
            ASSERT_TRUE(iter->hasNext());
            ASSERT_THAT(iter->next(), Eq(value));
          }
        }
        ASSERT_FALSE(iter->hasNext());
      } else {
        for (auto v = 0; v != GetParam(); ++v) {
          ASSERT_TRUE(iter->hasNext());
          ASSERT_THAT(iter->next(), Eq(std::to_string(v)));
        }
        ASSERT_FALSE(iter->hasNext());
      }
    }
  }

  auto partition = openOrCreateMapPartition(prefix);
  for (auto k = 0; k != GetParam(); ++k) {
    const auto key = std::to_string(k);
    auto iter = partition->get(key);
    if (is_odd(key)) {
      for (auto v = 0; v != GetParam(); ++v) {
        const auto value = std::to_string(v);
        if (is_odd(value)) {
          // This value must have been removed.
        } else {
          ASSERT_TRUE(iter->hasNext());
          ASSERT_THAT(iter->next(), Eq(value));
        }
      }
      ASSERT_FALSE(iter->hasNext());
    } else {
      for (auto v = 0; v != GetParam(); ++v) {
        ASSERT_TRUE(iter->hasNext());
        ASSERT_THAT(iter->next(), Eq(std::to_string(v)));
      }
      ASSERT_FALSE(iter->hasNext());
    }
  }
}

INSTANTIATE_TEST_CASE_P(Parameterized, MapPartitionTestWithParam,
                        testing::Values(0, 1, 2, 10, 100, 1000));

// INSTANTIATE_TEST_CASE_P(ParameterizedLongRunning, MapPartitionTestWithParam,
//                         testing::Values(10000, 100000));

// -----------------------------------------------------------------------------
// class MapPartition / Concurrency
// -----------------------------------------------------------------------------

const auto NULL_PROCEDURE = [](const Bytes&) {};
const auto TRUE_PREDICATE = [](const Bytes&) { return true; };
const auto EMPTY_FUNCTION = [](const Bytes&) { return std::string(); };

TEST_F(MapPartitionTestFixture, GetDifferentListsDoesNotBlock) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  partition->put(k2, v1);
  auto iter1 = partition->get(k1);
  ASSERT_TRUE(iter1->hasNext());
  auto iter2 = partition->get(k2);
  ASSERT_TRUE(iter2->hasNext());
}

TEST_F(MapPartitionTestFixture, GetSameListTwiceDoesNotBlock) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  auto iter1 = partition->get(k1);
  ASSERT_TRUE(iter1->hasNext());
  auto iter2 = partition->get(k1);
  ASSERT_TRUE(iter2->hasNext());
}

TEST_F(MapPartitionTestFixture, RemoveKeyBlocksIfListIsLocked) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  auto thread_is_running = true;
  {
    auto iter = partition->get(k1);
    ASSERT_TRUE(iter->hasNext());
    std::thread thread([&] {
      partition->removeKey(k1);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(MapPartitionTestFixture, RemoveKeysBlocksIfListIsLocked) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  auto thread_is_running = true;
  {
    auto iter = partition->get(k1);
    ASSERT_TRUE(iter->hasNext());
    std::thread thread([&] {
      partition->removeKeys(TRUE_PREDICATE);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(MapPartitionTestFixture, RemoveValueBlocksIfListIsLocked) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  auto thread_is_running = true;
  {
    auto iter = partition->get(k1);
    ASSERT_TRUE(iter->hasNext());
    std::thread thread([&] {
      partition->removeValue(k1, TRUE_PREDICATE);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(MapPartitionTestFixture, RemoveValuesBlocksIfListIsLocked) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  auto thread_is_running = true;
  {
    auto iter = partition->get(k1);
    ASSERT_TRUE(iter->hasNext());
    std::thread thread([&] {
      partition->removeValues(k1, TRUE_PREDICATE);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(MapPartitionTestFixture, ReplaceValueBlocksIfListIsLocked) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  auto thread_is_running = true;
  {
    auto iter = partition->get(k1);
    ASSERT_TRUE(iter->hasNext());
    std::thread thread([&] {
      partition->replaceValue(k1, EMPTY_FUNCTION);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(MapPartitionTestFixture, ReplaceValuesBlocksIfListIsLocked) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  auto thread_is_running = true;
  {
    auto iter = partition->get(k1);
    ASSERT_TRUE(iter->hasNext());
    std::thread thread([&] {
      partition->replaceValues(k1, EMPTY_FUNCTION);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(MapPartitionTestFixture, ForEachKeyDoesNotBlockIfListIsLocked) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  auto thread_is_running = true;
  std::thread thread([&] {
    partition->forEachKey(NULL_PROCEDURE);
    thread_is_running = false;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(MapPartitionTestFixture, ForEachValueDoesNotBlockIfListIsLocked) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  auto thread_is_running = true;
  std::thread thread([&] {
    partition->forEachValue(k1, NULL_PROCEDURE);
    thread_is_running = false;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(MapPartitionTestFixture, ForEachEntryDoesNotBlockIfListIsLocked) {
  auto partition = openOrCreateMapPartition(prefix);
  partition->put(k1, v1);
  auto thread_is_running = true;
  std::thread thread([&] {
    partition->forEachEntry([](const Bytes&, Iterator*) {});
    thread_is_running = false;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

// -----------------------------------------------------------------------------
// class MapPartition::Stats
// -----------------------------------------------------------------------------

TEST(MapPartitionStatsTest, NamesAndToVectorHaveSameDimension) {
  ASSERT_THAT(Stats::names().size(), Eq(Stats().toVector().size()));
}

}  // namespace internal
}  // namespace multimap
