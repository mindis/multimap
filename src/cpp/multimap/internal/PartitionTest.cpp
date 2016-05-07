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

#include <map>
#include <string>
#include <thread>  // NOLINT
#include <type_traits>
#include <vector>
#include <boost/filesystem/operations.hpp>  // NOLINT
#include "gmock/gmock.h"
#include "multimap/internal/Partition.h"

namespace multimap {
namespace internal {

using testing::Eq;
using testing::ElementsAre;
using testing::UnorderedElementsAre;

std::unique_ptr<Partition> openPartition(const std::string& prefix,
                                         const Options& options) {
  return std::unique_ptr<Partition>(new Partition(prefix, options));
}

std::unique_ptr<Partition> openOrCreatePartition(const std::string& prefix) {
  return std::unique_ptr<Partition>(new Partition(prefix, Options()));
}

std::unique_ptr<Partition> openOrCreatePartitionAsReadOnly(
    const std::string& prefix) {
  Options options;
  options.readonly = true;
  return std::unique_ptr<Partition>(new Partition(prefix, options));
}

TEST(PartitionTest, IsNotDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<Partition>::value);
}

TEST(PartitionTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<Partition>::value);
  ASSERT_FALSE(std::is_copy_assignable<Partition>::value);
}

TEST(PartitionTest, IsNotMoveConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_move_constructible<Partition>::value);
  ASSERT_FALSE(std::is_move_assignable<Partition>::value);
}

struct PartitionTestFixture : public testing::Test {
  void SetUp() override {
    boost::filesystem::remove_all(directory);
    boost::filesystem::create_directory(directory);
  }

  void TearDown() override { boost::filesystem::remove_all(directory); }

  const std::string directory = "/tmp/multimap.PartitionTestFixture";
  const std::string prefix = directory + "/partition";
  const Bytes k1 = toBytes("k1");
  const Bytes k2 = toBytes("k2");
  const Bytes k3 = toBytes("k3");
  const Bytes v1 = toBytes("v1");
  const Bytes v2 = toBytes("v2");
  const Bytes v3 = toBytes("v3");
  const std::vector<Bytes> keys = {k1, k2, k3};
  const std::vector<Bytes> values = {v1, v2, v3};
};

TEST_F(PartitionTestFixture, PutAppendsValueToList) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  partition->put(k1, v2);
  partition->put(k1, v3);
  auto iter = partition->get(k1);
  ASSERT_THAT(iter->next(), Eq(v1));
  ASSERT_THAT(iter->next(), Eq(v2));
  ASSERT_THAT(iter->next(), Eq(v3));
  ASSERT_FALSE(iter->hasNext());
}

TEST_F(PartitionTestFixture, PutMaxKeyDoesNotThrow) {
  auto partition = openOrCreatePartition(prefix);
  try {
    std::string key(Partition::Limits::maxKeySize(), 'k');
    ASSERT_NO_THROW(partition->put(key, v1));
  } catch (...) {
    // Allocating key itself may fail.
  }
}

TEST_F(PartitionTestFixture, PutTooBigKeyThrows) {
  auto partition = openOrCreatePartition(prefix);
  try {
    std::string key(Partition::Limits::maxKeySize() + 1, 'k');
    ASSERT_THROW(partition->put(key, v1), mt::AssertionError);
  } catch (...) {
    // Allocating key itself may fail.
  }
}

TEST_F(PartitionTestFixture, PutMaxValueDoesNotThrow) {
  auto partition = openOrCreatePartition(prefix);
  try {
    std::string value(Partition::Limits::maxValueSize(), 'v');
    ASSERT_NO_THROW(partition->put(k1, value));
  } catch (...) {
    // Allocating value itself may fail.
  }
}

TEST_F(PartitionTestFixture, PutTooBigValueThrows) {
  auto partition = openOrCreatePartition(prefix);
  try {
    std::string value(Partition::Limits::maxValueSize() + 1, 'v');
    ASSERT_THROW(partition->put(k1, value), mt::AssertionError);
  } catch (...) {
    // Allocating value itself may fail.
  }
}

TEST_F(PartitionTestFixture, PutValuesAndReopenInBetween) {
  {
    Options options;
    options.block_size = 128;
    auto partition = openPartition(prefix, options);
    partition->put(k1, v1);
    partition->put(k2, v1);
    partition->put(k3, v1);
  }
  {
    auto partition = openOrCreatePartition(prefix);
    partition->put(k1, v2);
    partition->put(k2, v2);
    partition->put(k3, v2);
  }
  {
    auto partition = openOrCreatePartition(prefix);
    partition->put(k1, v3);
    partition->put(k2, v3);
    partition->put(k3, v3);
  }
  auto partition = openOrCreatePartition(prefix);
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

TEST_F(PartitionTestFixture, GetReturnsEmptyIteratorForNonExistingKey) {
  auto partition = openOrCreatePartition(prefix);
  ASSERT_FALSE(partition->get(k1)->hasNext());
}

TEST_F(PartitionTestFixture, RemoveRemovesMatchingKeyAndItsValues) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  partition->put(k2, v1);
  partition->put(k3, v1);
  ASSERT_THAT(partition->remove(k1), Eq(1));
  ASSERT_THAT(partition->remove(k2), Eq(1));
  ASSERT_FALSE(partition->get(k1)->hasNext());
  ASSERT_FALSE(partition->get(k2)->hasNext());
  ASSERT_TRUE(partition->get(k3)->hasNext());
}

TEST_F(PartitionTestFixture, RemoveFirstMatchRemovesFirstMatchingKey) {
  auto partition = openOrCreatePartition(prefix);
  for (const Bytes& key : keys) {
    partition->put(key, values.begin(), values.end());
  }
  Predicate is_k1_or_k2 =
      [&](const Slice& key) { return key == k1 || key == k2; };  // NOLINT
  ASSERT_THAT(partition->removeFirstMatch(is_k1_or_k2), Eq(values.size()));
  if (partition->get(k1)->hasNext()) {
    ASSERT_THAT(partition->get(k1)->available(), Eq(values.size()));
    ASSERT_THAT(partition->get(k2)->available(), Eq(0));
  }
  if (partition->get(k2)->hasNext()) {
    ASSERT_THAT(partition->get(k2)->available(), Eq(values.size()));
    ASSERT_THAT(partition->get(k1)->available(), Eq(0));
  }
  ASSERT_THAT(partition->get(k3)->available(), Eq(values.size()));
}

TEST_F(PartitionTestFixture, RemoveAllMatchesRemovesAllMatchingKeys) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  partition->put(k2, v1);
  partition->put(k3, v1);
  Predicate is_k1_or_k2 =
      [&](const Slice& key) { return key == k1 || key == k2; };  // NOLINT
  auto result = partition->removeAllMatches(is_k1_or_k2);
  ASSERT_THAT(result.first, Eq(2));
  ASSERT_THAT(result.second, Eq(2));
  ASSERT_FALSE(partition->get(k1)->hasNext());
  ASSERT_FALSE(partition->get(k2)->hasNext());
  ASSERT_TRUE(partition->get(k3)->hasNext());
}

TEST_F(PartitionTestFixture, RemoveFirstMatchInListRemovesFirstMatchingValue) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  partition->put(k1, v2);
  partition->put(k1, v3);
  Predicate is_any = [&](const Slice& /* value */) { return true; };  // NOLINT
  ASSERT_TRUE(partition->removeFirstMatch(k1, is_any));
  auto iter = partition->get(k1);
  ASSERT_THAT(iter->next(), Eq(v2));
  ASSERT_THAT(iter->next(), Eq(v3));
  ASSERT_FALSE(iter->hasNext());

  partition->put(k2, v1);
  partition->put(k2, v2);
  partition->put(k2, v3);
  Predicate is_v2_or_v3 = [&](const Slice& value) {  // NOLINT
    return value == v2 || value == v3;
  };
  ASSERT_TRUE(partition->removeFirstMatch(k2, is_v2_or_v3));
  iter = partition->get(k2);
  ASSERT_THAT(iter->next(), Eq(v1));
  ASSERT_THAT(iter->next(), Eq(v3));
  ASSERT_FALSE(iter->hasNext());
}

TEST_F(PartitionTestFixture, RemoveAllMatchesInListRemovesAllMatchingValues) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  partition->put(k1, v2);
  partition->put(k1, v3);
  Predicate is_any = [&](const Slice& /* value */) { return true; };  // NOLINT
  ASSERT_THAT(partition->removeAllMatches(k1, is_any), Eq(3));
  auto iter = partition->get(k1);
  ASSERT_FALSE(iter->hasNext());

  partition->put(k2, v1);
  partition->put(k2, v2);
  partition->put(k2, v3);
  Predicate is_v2_or_v3 = [&](const Slice& value) {  // NOLINT
    return value == v2 || value == v3;
  };
  ASSERT_TRUE(partition->removeAllMatches(k2, is_v2_or_v3));
  iter = partition->get(k2);
  ASSERT_THAT(iter->next(), Eq(v1));
  ASSERT_FALSE(iter->hasNext());
}

TEST_F(PartitionTestFixture, ReplaceFirstMatchReplacesFirstMatchingValue) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  partition->put(k1, v2);
  partition->put(k1, v3);
  Function rotate = [&](const Slice& value) {  // NOLINT
    if (value == v1) return v2;
    if (value == v2) return v3;
    if (value == v3) return v1;
    return Bytes();
  };
  ASSERT_TRUE(partition->replaceFirstMatch(k1, rotate));
  auto iter = partition->get(k1);
  ASSERT_THAT(iter->next(), Eq(v2));
  ASSERT_THAT(iter->next(), Eq(v3));
  ASSERT_THAT(iter->next(), Eq(v2));  // v1 replacement
  ASSERT_FALSE(iter->hasNext());

  partition->put(k2, v1);
  partition->put(k2, v2);
  partition->put(k2, v3);
  Function rotate_v2_or_v3 = [&](const Slice& value) {  // NOLINT
    if (value == v2) return v3;
    if (value == v3) return v1;
    return Bytes();
  };
  ASSERT_TRUE(partition->replaceFirstMatch(k2, rotate_v2_or_v3));
  iter = partition->get(k2);
  ASSERT_THAT(iter->next(), Eq(v1));
  ASSERT_THAT(iter->next(), Eq(v3));
  ASSERT_THAT(iter->next(), Eq(v3));  // v2 replacement
  ASSERT_FALSE(iter->hasNext());
}

TEST_F(PartitionTestFixture, ReplaceAllMatchesReplacesAllMatchingValues) {
  auto partition = openOrCreatePartition(prefix);
  for (const Bytes& key : keys) {
    partition->put(key, values.begin(), values.end());
  }
  Function rotate = [&](const Slice& value) {  // NOLINT
    if (value == v1) return v2;
    if (value == v2) return v3;
    if (value == v3) return v1;
    return Bytes();
  };
  ASSERT_THAT(partition->replaceAllMatches(k1, rotate), Eq(3));
  auto iter = partition->get(k1);
  ASSERT_THAT(iter->next(), Eq(v2));  // v1 replacement
  ASSERT_THAT(iter->next(), Eq(v3));  // v2 replacement
  ASSERT_THAT(iter->next(), Eq(v1));  // v3 replacement
  ASSERT_FALSE(iter->hasNext());

  Function rotate_v2_or_v3 = [&](const Slice& value) {  // NOLINT
    if (value == v2) return v3;
    if (value == v3) return v1;
    return Bytes();
  };
  ASSERT_THAT(partition->replaceAllMatches(k2, rotate_v2_or_v3), Eq(2));
  iter = partition->get(k2);
  ASSERT_THAT(iter->next(), Eq(v1));
  ASSERT_THAT(iter->next(), Eq(v3));  // v2 replacement
  ASSERT_THAT(iter->next(), Eq(v1));  // v3 replacement
  ASSERT_FALSE(iter->hasNext());
}

TEST_F(PartitionTestFixture, ForEachKeyIgnoresEmptyLists) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  partition->put(k2, v1);
  partition->put(k3, v1);
  std::vector<Bytes> keys;
  Procedure collect = [&](const Slice& key) {  // NOLINT
    keys.push_back(key.makeCopy());
  };
  partition->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k1, k2, k3));

  keys.clear();
  partition->remove(k1);
  partition->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k2, k3));

  keys.clear();
  partition->remove(k2);
  partition->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k3));

  keys.clear();
  partition->remove(k3);
  partition->forEachKey(collect);
  ASSERT_TRUE(keys.empty());

  partition->put(k1, v1);
  partition->put(k2, v1);
  partition->put(k3, v1);
  partition->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k1, k2, k3));

  Predicate is_any = [](const Slice& /* value */) { return true; };

  keys.clear();
  partition->removeAllMatches(k1, is_any);
  partition->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k2, k3));

  keys.clear();
  partition->removeAllMatches(k2, is_any);
  partition->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k3));

  keys.clear();
  partition->removeAllMatches(k3, is_any);
  partition->forEachKey(collect);
  ASSERT_TRUE(keys.empty());
}

TEST_F(PartitionTestFixture, ForEachValueVisitsAllValues) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  partition->put(k2, v1);
  partition->put(k2, v2);
  std::vector<Bytes> values;
  Procedure collect = [&](const Slice& value) {  // NOLINT
    values.push_back(value.makeCopy());
  };
  partition->forEachValue(k1, collect);
  ASSERT_THAT(values, UnorderedElementsAre(v1));

  values.clear();
  partition->forEachValue(k2, collect);
  ASSERT_THAT(values, UnorderedElementsAre(v1, v2));

  values.clear();
  partition->forEachValue(k3, collect);
  ASSERT_TRUE(values.empty());
}

TEST_F(PartitionTestFixture, ForEachEntryIgnoresEmptyLists) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  partition->put(k2, v1);
  partition->put(k2, v2);
  partition->put(k3, v1);
  partition->put(k3, v2);
  partition->put(k3, v3);
  std::map<Bytes, std::vector<Bytes>, BytesLess> mapping;
  BinaryProcedure collect = [&](const Slice& key, Iterator* iter) {  // NOLINT
    while (iter->hasNext()) {
      mapping[key.makeCopy()].push_back(iter->next().makeCopy());
    }
  };
  partition->forEachEntry(collect);
  ASSERT_THAT(mapping.size(), Eq(3));
  ASSERT_THAT(mapping.at(k1), ElementsAre(v1));
  ASSERT_THAT(mapping.at(k2), ElementsAre(v1, v2));
  ASSERT_THAT(mapping.at(k3), ElementsAre(v1, v2, v3));

  mapping.clear();
  partition->removeAllMatches(k2,
                              [](const Slice& /* value */) { return true; });
  partition->forEachEntry(collect);
  ASSERT_THAT(mapping.size(), Eq(2));
  ASSERT_THAT(mapping.at(k1), ElementsAre(v1));
  ASSERT_THAT(mapping.at(k3), ElementsAre(v1, v2, v3));
}

TEST_F(PartitionTestFixture, GetStatsReturnsCorrectValues) {
  auto partition = openOrCreatePartition(prefix);
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

  Stats stats = partition->getStats();
  ASSERT_THAT(stats.block_size, Eq(Options().block_size));
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

  partition->remove("kkkkk");
  stats = partition->getStats();
  ASSERT_THAT(stats.block_size, Eq(Options().block_size));
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

TEST_F(PartitionTestFixture, GetStatsReturnsCorrectValuesAfterRemovingKeys) {
  {
    auto partition = openOrCreatePartition(prefix);
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

    partition->remove("k");
    partition->removeFirstMatch([](const Slice& key) { return key == "kk"; });

    const Stats stats = partition->getStats();
    ASSERT_THAT(stats.num_keys_total, Eq(5));
    ASSERT_THAT(stats.num_keys_valid, Eq(3));
    ASSERT_THAT(stats.num_values_total, Eq(15));
    ASSERT_THAT(stats.num_values_valid, Eq(12));
  }

  auto partition = openOrCreatePartition(prefix);
  const Stats stats = partition->getStats();
  ASSERT_THAT(stats.num_keys_total, Eq(3));
  ASSERT_THAT(stats.num_keys_valid, Eq(3));
  ASSERT_THAT(stats.num_values_total, Eq(15));
  ASSERT_THAT(stats.num_values_valid, Eq(12));
}

TEST_F(PartitionTestFixture, GetStatsReturnsCorrectValuesAfterRemovingValues) {
  {
    auto partition = openOrCreatePartition(prefix);
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

    partition->removeFirstEqual("k", "vvvvv");
    partition->removeAllEqual("kk", "vvvv");

    const Stats stats = partition->getStats();
    ASSERT_THAT(stats.num_keys_total, Eq(5));
    ASSERT_THAT(stats.num_keys_valid, Eq(3));
    ASSERT_THAT(stats.num_values_total, Eq(15));
    ASSERT_THAT(stats.num_values_valid, Eq(12));
  }

  auto partition = openOrCreatePartition(prefix);
  const Stats stats = partition->getStats();
  ASSERT_THAT(stats.num_keys_total, Eq(3));
  ASSERT_THAT(stats.num_keys_valid, Eq(3));
  ASSERT_THAT(stats.num_values_total, Eq(15));
  ASSERT_THAT(stats.num_values_valid, Eq(12));
}

TEST_F(PartitionTestFixture, PutThrowsIfOpenedAsReadOnly) {
  auto partition = openOrCreatePartitionAsReadOnly(prefix);
  ASSERT_THROW(partition->put(k1, v1), std::runtime_error);
}

TEST_F(PartitionTestFixture, RemoveThrowsIfOpenedAsReadOnly) {
  auto partition = openOrCreatePartitionAsReadOnly(prefix);
  ASSERT_THROW(partition->remove(k1), std::runtime_error);
}

TEST_F(PartitionTestFixture, RemoveFirstMatchThrowsIfOpenedAsReadOnly) {
  auto partition = openOrCreatePartitionAsReadOnly(prefix);
  ASSERT_THROW(
      partition->removeFirstMatch([](const Slice& /* key */) { return true; }),
      std::runtime_error);
}

TEST_F(PartitionTestFixture, RemoveAllMatchesThrowsIfOpenedAsReadOnly) {
  auto partition = openOrCreatePartitionAsReadOnly(prefix);
  ASSERT_THROW(
      partition->removeAllMatches([](const Slice& /* key */) { return true; }),
      std::runtime_error);
}

TEST_F(PartitionTestFixture, RemoveFirstEqualInListThrowsIfOpenedAsReadOnly) {
  auto partition = openOrCreatePartitionAsReadOnly(prefix);
  ASSERT_THROW(partition->removeFirstEqual(k1, v1), std::runtime_error);
}

TEST_F(PartitionTestFixture, RemoveAllEqualInListThrowsIfOpenedAsReadOnly) {
  auto partition = openOrCreatePartitionAsReadOnly(prefix);
  ASSERT_THROW(partition->removeAllEqual(k1, v1), std::runtime_error);
}

TEST_F(PartitionTestFixture, ReplaceFirstEqualThrowsIfOpenedAsReadOnly) {
  auto partition = openOrCreatePartitionAsReadOnly(prefix);
  ASSERT_THROW(partition->replaceFirstEqual(k1, v1, v2), std::runtime_error);
}

TEST_F(PartitionTestFixture, ReplaceAllEqualThrowsIfOpenedAsReadOnly) {
  auto partition = openOrCreatePartitionAsReadOnly(prefix);
  ASSERT_THROW(partition->replaceAllEqual(k1, v1, v2), std::runtime_error);
}

// -----------------------------------------------------------------------------
// class Partition / Mutability
// -----------------------------------------------------------------------------

struct PartitionTestWithParam : public testing::TestWithParam<int> {
  void SetUp() override {
    boost::filesystem::remove_all(directory);
    boost::filesystem::create_directory(directory);
  }

  void TearDown() override { boost::filesystem::remove_all(directory); }

  const std::string directory = "/tmp/multimap.PartitionTestWithParam";
  const std::string prefix = directory + "/partition";
};

TEST_P(PartitionTestWithParam, PutDataThenReadAll) {
  {
    auto partition = openOrCreatePartition(prefix);
    for (int k = 0; k != GetParam(); ++k) {
      for (int v = 0; v != GetParam(); ++v) {
        partition->put(std::to_string(k), std::to_string(v));
      }
    }

    for (int k = 0; k != GetParam(); ++k) {
      auto iter = partition->get(std::to_string(k));
      for (int v = 0; v != GetParam(); ++v) {
        ASSERT_TRUE(iter->hasNext());
        ASSERT_THAT(iter->next(), std::to_string(v));
      }
      ASSERT_FALSE(iter->hasNext());
    }
  }

  auto partition = openOrCreatePartition(prefix);
  for (int k = 0; k != GetParam(); ++k) {
    auto iter = partition->get(std::to_string(k));
    for (int v = 0; v != GetParam(); ++v) {
      ASSERT_TRUE(iter->hasNext());
      ASSERT_THAT(iter->next(), std::to_string(v));
    }
    ASSERT_FALSE(iter->hasNext());
  }
}

TEST_P(PartitionTestWithParam, PutDataThenRemoveSomeThenReadAll) {
  Predicate is_odd =
      [](const Slice& slice) { return std::stoul(slice.toString()) % 2; };
  {
    auto partition = openOrCreatePartition(prefix);
    for (int k = 0; k != GetParam(); ++k) {
      for (int v = 0; v != GetParam(); ++v) {
        partition->put(std::to_string(k), std::to_string(v));
      }
    }

    for (int k = 0; k != GetParam(); ++k) {
      const auto key = std::to_string(k);
      if (is_odd(key)) {
        partition->removeAllMatches(key, is_odd);
      }
    }

    for (int k = 0; k != GetParam(); ++k) {
      const auto key = std::to_string(k);
      auto iter = partition->get(key);
      if (is_odd(key)) {
        for (int v = 0; v != GetParam(); ++v) {
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
        for (int v = 0; v != GetParam(); ++v) {
          ASSERT_TRUE(iter->hasNext());
          ASSERT_THAT(iter->next(), Eq(std::to_string(v)));
        }
        ASSERT_FALSE(iter->hasNext());
      }
    }
  }

  auto partition = openOrCreatePartition(prefix);
  for (int k = 0; k != GetParam(); ++k) {
    const auto key = std::to_string(k);
    auto iter = partition->get(key);
    if (is_odd(key)) {
      for (int v = 0; v != GetParam(); ++v) {
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
      for (int v = 0; v != GetParam(); ++v) {
        ASSERT_TRUE(iter->hasNext());
        ASSERT_THAT(iter->next(), Eq(std::to_string(v)));
      }
      ASSERT_FALSE(iter->hasNext());
    }
  }
}

INSTANTIATE_TEST_CASE_P(Parameterized, PartitionTestWithParam,
                        testing::Values(0, 1, 2, 10, 100, 1000));

// INSTANTIATE_TEST_CASE_P(ParameterizedLongRunning, PartitionTestWithParam,
//                         testing::Values(10000, 100000));

// -----------------------------------------------------------------------------
// class Partition / Concurrency
// -----------------------------------------------------------------------------

const Procedure NULL_PROCEDURE = [](const Slice&) {};
const Predicate TRUE_PREDICATE = [](const Slice&) { return true; };
const Function EMPTY_FUNCTION = [](const Slice&) { return Bytes(); };

TEST_F(PartitionTestFixture, GetDifferentListsDoesNotBlock) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  partition->put(k2, v1);
  auto iter1 = partition->get(k1);
  ASSERT_TRUE(iter1->hasNext());
  auto iter2 = partition->get(k2);
  ASSERT_TRUE(iter2->hasNext());
}

TEST_F(PartitionTestFixture, GetSameListTwiceDoesNotBlock) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  auto iter1 = partition->get(k1);
  ASSERT_TRUE(iter1->hasNext());
  auto iter2 = partition->get(k1);
  ASSERT_TRUE(iter2->hasNext());
}

TEST_F(PartitionTestFixture, RemoveBlocksIfListIsLocked) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  bool thread_is_running = true;
  {
    auto iter = partition->get(k1);
    ASSERT_TRUE(iter->hasNext());
    std::thread thread([&] {  // NOLINT
      partition->remove(k1);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(PartitionTestFixture, RemoveFirstMatchBlocksIfListIsLocked) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  bool thread_is_running = true;
  {
    auto iter = partition->get(k1);
    ASSERT_TRUE(iter->hasNext());
    std::thread thread([&] {  // NOLINT
      partition->removeFirstMatch(TRUE_PREDICATE);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(PartitionTestFixture, RemoveAllMatchesBlocksIfListIsLocked) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  bool thread_is_running = true;
  {
    auto iter = partition->get(k1);
    ASSERT_TRUE(iter->hasNext());
    std::thread thread([&] {  // NOLINT
      partition->removeAllMatches(TRUE_PREDICATE);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(PartitionTestFixture, RemoveFirstMatchInListBlocksIfListIsLocked) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  bool thread_is_running = true;
  {
    auto iter = partition->get(k1);
    ASSERT_TRUE(iter->hasNext());
    std::thread thread([&] {  // NOLINT
      partition->removeFirstMatch(k1, TRUE_PREDICATE);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(PartitionTestFixture, RemoveAllMatchesInListBlocksIfListIsLocked) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  bool thread_is_running = true;
  {
    auto iter = partition->get(k1);
    ASSERT_TRUE(iter->hasNext());
    std::thread thread([&] {  // NOLINT
      partition->removeAllMatches(k1, TRUE_PREDICATE);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(PartitionTestFixture, ReplaceFirstMatchBlocksIfListIsLocked) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  bool thread_is_running = true;
  {
    auto iter = partition->get(k1);
    ASSERT_TRUE(iter->hasNext());
    std::thread thread([&] {  // NOLINT
      partition->replaceFirstMatch(k1, EMPTY_FUNCTION);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(PartitionTestFixture, ReplaceAllMatchesBlocksIfListIsLocked) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  bool thread_is_running = true;
  {
    auto iter = partition->get(k1);
    ASSERT_TRUE(iter->hasNext());
    std::thread thread([&] {  // NOLINT
      partition->replaceAllMatches(k1, EMPTY_FUNCTION);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(PartitionTestFixture, ForEachKeyDoesNotBlockIfListIsLocked) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  bool thread_is_running = true;
  std::thread thread([&] {  // NOLINT
    partition->forEachKey(NULL_PROCEDURE);
    thread_is_running = false;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(PartitionTestFixture, ForEachValueDoesNotBlockIfListIsLocked) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  bool thread_is_running = true;
  std::thread thread([&] {  // NOLINT
    partition->forEachValue(k1, NULL_PROCEDURE);
    thread_is_running = false;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(PartitionTestFixture, ForEachEntryDoesNotBlockIfListIsLocked) {
  auto partition = openOrCreatePartition(prefix);
  partition->put(k1, v1);
  bool thread_is_running = true;
  std::thread thread([&] {  // NOLINT
    partition->forEachEntry([](const Slice&, Iterator*) {});
    thread_is_running = false;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

// -----------------------------------------------------------------------------
// class Partition::Stats
// -----------------------------------------------------------------------------

TEST(PartitionStatsTest, NamesAndToVectorHaveSameDimension) {
  ASSERT_THAT(Stats::names().size(), Eq(Stats().toVector().size()));
}

}  // namespace internal
}  // namespace multimap
