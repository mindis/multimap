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

#include <algorithm>
#include <set>
#include <type_traits>
#include <boost/filesystem/operations.hpp>
#include "multimap/internal/MphTable.hpp"
#include "multimap/thirdparty/mt/assert.hpp"
#include "gmock/gmock.h"

namespace multimap {
namespace internal {

namespace {

Stats buildMphTable(const std::string& prefix, const Options& options,
                    int num_keys, int num_values) {
  MphTable::Builder builder(prefix, options);
  for (int k = 0; k < num_keys; k++) {
    const auto key = std::to_string(k);
    for (int v = 0; v < num_values; v++) {
      builder.put(key, std::to_string(v));
    }
  }
  return builder.build();
}

}  // namespace

// -----------------------------------------------------------------------------
// class MphTable::Builder
// -----------------------------------------------------------------------------

TEST(MphTableBuilderTest, IsNotDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<MphTable::Builder>::value);
}

TEST(MphTableBuilderTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<MphTable::Builder>::value);
  ASSERT_FALSE(std::is_copy_assignable<MphTable::Builder>::value);
}

TEST(MphTableBuilderTest, IsMoveConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_move_constructible<MphTable::Builder>::value);
  ASSERT_TRUE(std::is_move_assignable<MphTable::Builder>::value);
}

class MphTableBuilderFixture : public testing::Test {
 public:
  void SetUp() override {
    directory_ = "/tmp/multimap.MphTableBuilderFixture";
    boost::filesystem::remove_all(directory_);
    MT_ASSERT_TRUE(boost::filesystem::create_directory(directory_));
    prefix_ = (directory_ / "builder").string();
  }

  void TearDown() override {
    MT_ASSERT_TRUE(boost::filesystem::remove_all(directory_));
  }

  const std::string& getPrefix() { return prefix_; }

 private:
  boost::filesystem::path directory_;
  std::string prefix_;
};

class MphTableBuilderTestWithParam : public testing::TestWithParam<int> {
 public:
  void SetUp() override {
    directory_ = "/tmp/multimap.MphTableBuilderTestWithParam";
    boost::filesystem::remove_all(directory_);
    MT_ASSERT_TRUE(boost::filesystem::create_directory(directory_));
    prefix_ = (directory_ / "builder").string();
  }

  void TearDown() override {
    MT_ASSERT_TRUE(boost::filesystem::remove_all(directory_));
  }

  const std::string& getPrefix() { return prefix_; }

 private:
  boost::filesystem::path directory_;
  std::string prefix_;
};

TEST_P(MphTableBuilderTestWithParam, PutDataAndBuild) {
  Options options;
  options.verbose = false;
  const Stats stats =
      buildMphTable(getPrefix(), options, GetParam(), GetParam());

  ASSERT_GE(stats.key_size_avg, stats.key_size_min);
  ASSERT_LE(stats.key_size_avg, stats.key_size_max);
  ASSERT_EQ(std::to_string(GetParam() - 1).size(), stats.key_size_max);
  ASSERT_EQ(1, stats.key_size_min);
  ASSERT_EQ(GetParam(), stats.list_size_avg);
  ASSERT_EQ(GetParam(), stats.list_size_max);
  ASSERT_EQ(GetParam(), stats.list_size_min);
  ASSERT_EQ(GetParam(), stats.num_keys_total);
  ASSERT_EQ(GetParam(), stats.num_keys_valid);
  ASSERT_EQ(0, stats.num_partitions);
  ASSERT_EQ(GetParam() * GetParam(), stats.num_values_total);
  ASSERT_EQ(GetParam() * GetParam(), stats.num_values_valid);
}

INSTANTIATE_TEST_CASE_P(Parameterized, MphTableBuilderTestWithParam,
                        testing::Values(10, 100, 1000));
// CMPH does not work for very small keysets, i.e. less than 10.

// -----------------------------------------------------------------------------
// class MphTable
// -----------------------------------------------------------------------------

TEST(MphTableTest, IsNotDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<MphTable>::value);
}

TEST(MphTableTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<MphTable>::value);
  ASSERT_FALSE(std::is_copy_assignable<MphTable>::value);
}

TEST(MphTableTest, IsMoveConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_move_constructible<MphTable>::value);
  ASSERT_TRUE(std::is_move_assignable<MphTable>::value);
}

TEST(MphTableTest, ConstructorWithEmptyPrefixThrows) {
  ASSERT_THROW(MphTable(""), std::runtime_error);
}

TEST(MphTableTest, ConstructorWithNonExistingPrefixThrows) {
  const std::string dir = "/abc";
  ASSERT_FALSE(boost::filesystem::is_directory(dir));
  ASSERT_THROW(MphTable(dir + "/prefix"), std::runtime_error);
}

class MphTableTestWithParam : public testing::TestWithParam<int> {
 public:
  void SetUp() override {
    directory_ = "/tmp/multimap.MphTableTestWithParam";
    boost::filesystem::remove_all(directory_);
    MT_ASSERT_TRUE(boost::filesystem::create_directory(directory_));
    prefix_ = (directory_ / "builder").string();
  }

  void TearDown() override {
    MT_ASSERT_TRUE(boost::filesystem::remove_all(directory_));
  }

  const std::string& getPrefix() { return prefix_; }

 private:
  boost::filesystem::path directory_;
  std::string prefix_;
};

TEST_P(MphTableTestWithParam, GetAndIterateEachListOnce) {
  Options options;
  options.verbose = false;
  buildMphTable(getPrefix(), options, GetParam(), GetParam());

  MphTable table(getPrefix());
  for (int k = 0; k < GetParam(); k++) {
    auto iter = table.get(std::to_string(k));
    for (int v = 0; v < GetParam(); v++) {
      ASSERT_TRUE(iter->hasNext());
      ASSERT_EQ(GetParam() - v, iter->available());
      ASSERT_EQ(std::to_string(v), iter->next());
    }
  }
}

TEST_P(MphTableTestWithParam, GetAndIterateEachListTwice) {
  Options options;
  options.verbose = false;
  buildMphTable(getPrefix(), options, GetParam(), GetParam());

  MphTable table(getPrefix());
  for (int k = 0; k < GetParam(); k++) {
    auto iter = table.get(std::to_string(k));
    for (int v = 0; v < GetParam(); v++) {
      ASSERT_TRUE(iter->hasNext());
      ASSERT_EQ(GetParam() - v, iter->available());
      ASSERT_EQ(std::to_string(v), iter->next());
    }
  }
  for (int k = 0; k < GetParam(); k++) {
    auto iter = table.get(std::to_string(k));
    for (int v = 0; v < GetParam(); v++) {
      ASSERT_TRUE(iter->hasNext());
      ASSERT_EQ(GetParam() - v, iter->available());
      ASSERT_EQ(std::to_string(v), iter->next());
    }
  }
}

TEST_P(MphTableTestWithParam, ForEachKeyVisitsAllKeys) {
  Options options;
  options.verbose = false;
  buildMphTable(getPrefix(), options, GetParam(), GetParam());

  std::set<int> keys;
  MphTable table(getPrefix());
  table.forEachKey(
      [&keys](const Slice& key) { keys.insert(std::stoi(key.toString())); });
  ASSERT_EQ(GetParam(), keys.size());
  int expected_key = 0;
  for (auto key : keys) {
    ASSERT_EQ(expected_key, key);
    expected_key++;
    // std::map is sorted.
  }
}

TEST_P(MphTableTestWithParam, ForEachValueVisitsAllValues) {
  Options options;
  options.verbose = false;
  buildMphTable(getPrefix(), options, GetParam(), GetParam());

  std::set<int> values;
  MphTable table(getPrefix());
  for (int k = 0; k < GetParam(); k++) {
    values.clear();
    table.forEachValue(std::to_string(k), [&values](const Slice& value) {
      values.insert(std::stoi(value.toString()));
    });
    ASSERT_EQ(GetParam(), values.size());
    int expected_value = 0;
    for (auto value : values) {
      ASSERT_EQ(expected_value, value);
      expected_value++;
      // std::map is sorted.
    }
  }
}

TEST_P(MphTableTestWithParam, ForEachEntryVisitsAllEntries) {
  Options options;
  options.verbose = false;
  buildMphTable(getPrefix(), options, GetParam(), GetParam());

  std::set<int> keys;
  MphTable table(getPrefix());
  table.forEachEntry([&keys, this](const Slice& key, Iterator* iter) {
    keys.insert(std::stoi(key.toString()));
    std::set<int> values;
    while (iter->hasNext()) {
      values.insert(std::stoi(iter->next().toString()));
    }
    ASSERT_EQ(GetParam(), values.size());
    int expected_value = 0;
    for (auto value : values) {
      ASSERT_EQ(expected_value, value);
      expected_value++;
      // std::map is sorted.
    }
  });
  ASSERT_EQ(GetParam(), keys.size());
  int expected_key = 0;
  for (auto key : keys) {
    ASSERT_EQ(expected_key, key);
    expected_key++;
    // std::map is sorted.
  }
}

TEST_P(MphTableTestWithParam, StaticForEachEntryVisitsAllEntries) {
  Options options;
  options.verbose = false;
  buildMphTable(getPrefix(), options, GetParam(), GetParam());

  std::set<int> keys;
  const auto process = [&keys, this](const Slice& key, Iterator* iter) {
    keys.insert(std::stoi(key.toString()));
    std::set<int> values;
    while (iter->hasNext()) {
      values.insert(std::stoi(iter->next().toString()));
    }
    ASSERT_EQ(GetParam(), values.size());
    int expected_value = 0;
    for (auto value : values) {
      ASSERT_EQ(expected_value, value);
      expected_value++;
      // std::map is sorted.
    }
  };

  MphTable::forEachEntry(getPrefix(), process);
  ASSERT_EQ(GetParam(), keys.size());
  int expected_key = 0;
  for (auto key : keys) {
    ASSERT_EQ(expected_key, key);
    expected_key++;
    // std::map is sorted.
  }
}

TEST_P(MphTableTestWithParam, GetStatsReturnsCorrectStats) {
  Options options;
  options.verbose = false;
  buildMphTable(getPrefix(), options, GetParam(), GetParam());

  MphTable table(getPrefix());
  const Stats stats = table.getStats();
  ASSERT_GE(stats.key_size_avg, stats.key_size_min);
  ASSERT_LE(stats.key_size_avg, stats.key_size_max);
  ASSERT_EQ(std::to_string(GetParam() - 1).size(), stats.key_size_max);
  ASSERT_EQ(1, stats.key_size_min);
  ASSERT_EQ(GetParam(), stats.list_size_avg);
  ASSERT_EQ(GetParam(), stats.list_size_max);
  ASSERT_EQ(GetParam(), stats.list_size_min);
  ASSERT_EQ(GetParam(), stats.num_keys_total);
  ASSERT_EQ(GetParam(), stats.num_keys_valid);
  ASSERT_EQ(0, stats.num_partitions);
  ASSERT_EQ(GetParam() * GetParam(), stats.num_values_total);
  ASSERT_EQ(GetParam() * GetParam(), stats.num_values_valid);
}

TEST_P(MphTableTestWithParam, StaticStatsReturnsCorrectStats) {
  Options options;
  options.verbose = false;
  buildMphTable(getPrefix(), options, GetParam(), GetParam());

  const Stats stats = MphTable::stats(getPrefix());
  ASSERT_GE(stats.key_size_avg, stats.key_size_min);
  ASSERT_LE(stats.key_size_avg, stats.key_size_max);
  ASSERT_EQ(std::to_string(GetParam() - 1).size(), stats.key_size_max);
  ASSERT_EQ(1, stats.key_size_min);
  ASSERT_EQ(GetParam(), stats.list_size_avg);
  ASSERT_EQ(GetParam(), stats.list_size_max);
  ASSERT_EQ(GetParam(), stats.list_size_min);
  ASSERT_EQ(GetParam(), stats.num_keys_total);
  ASSERT_EQ(GetParam(), stats.num_keys_valid);
  ASSERT_EQ(0, stats.num_partitions);
  ASSERT_EQ(GetParam() * GetParam(), stats.num_values_total);
  ASSERT_EQ(GetParam() * GetParam(), stats.num_values_valid);
}

TEST_P(MphTableTestWithParam, BuildWithListSortingAndIterateOnce) {
  Options options;
  options.verbose = false;
  options.compare = [](const Slice& a, const Slice& b) {
    return std::stoi(a.toString()) > std::stoi(b.toString());
  };
  buildMphTable(getPrefix(), options, GetParam(), GetParam());

  MphTable table(getPrefix());
  for (int k = 0; k < GetParam(); k++) {
    auto iter = table.get(std::to_string(k));
    for (int v = 0; v < GetParam(); v++) {
      ASSERT_TRUE(iter->hasNext());
      ASSERT_EQ(GetParam() - v, iter->available());
      ASSERT_EQ(std::to_string(GetParam() - v - 1), iter->next());
    }
  }
}

TEST_P(MphTableTestWithParam, BuildWithListSortingAndIterateTwice) {
  Options options;
  options.verbose = false;
  options.compare = [](const Slice& a, const Slice& b) {
    return std::stoi(a.toString()) > std::stoi(b.toString());
  };
  buildMphTable(getPrefix(), options, GetParam(), GetParam());

  MphTable table(getPrefix());
  for (int k = 0; k < GetParam(); k++) {
    auto iter = table.get(std::to_string(k));
    for (int v = 0; v < GetParam(); v++) {
      ASSERT_TRUE(iter->hasNext());
      ASSERT_EQ(GetParam() - v, iter->available());
      ASSERT_EQ(std::to_string(GetParam() - v - 1), iter->next());
    }
  }
  for (int k = 0; k < GetParam(); k++) {
    auto iter = table.get(std::to_string(k));
    for (int v = 0; v < GetParam(); v++) {
      ASSERT_TRUE(iter->hasNext());
      ASSERT_EQ(GetParam() - v, iter->available());
      ASSERT_EQ(std::to_string(GetParam() - v - 1), iter->next());
    }
  }
}

INSTANTIATE_TEST_CASE_P(Parameterized, MphTableTestWithParam,
                        testing::Values(10, 100, 1000));
// CMPH does not work for very small keysets, i.e. less than 10.

#ifdef MULTIMAP_RUN_LARGE_TESTS

TEST_F(MphTableBuilderFixture, PutTenMillionKeysThenBuildAndIterate) {
  Options options;
  options.verbose = true;
  const int num_keys = mt::MiB(10);
  const int num_values = 1;
  const Stats stats = buildMphTable(getPrefix(), options, num_keys, num_values);
  ASSERT_GE(stats.key_size_avg, stats.key_size_min);
  ASSERT_LE(stats.key_size_avg, stats.key_size_max);
  ASSERT_EQ(std::to_string(num_keys - 1).size(), stats.key_size_max);
  ASSERT_EQ(1, stats.key_size_min);
  ASSERT_EQ(num_values, stats.list_size_avg);
  ASSERT_EQ(num_values, stats.list_size_max);
  ASSERT_EQ(num_values, stats.list_size_min);
  ASSERT_EQ(num_keys, stats.num_keys_total);
  ASSERT_EQ(num_keys, stats.num_keys_valid);
  ASSERT_EQ(0, stats.num_partitions);
  ASSERT_EQ(num_keys * num_values, stats.num_values_total);
  ASSERT_EQ(num_keys * num_values, stats.num_values_valid);

  MphTable table(getPrefix());
  for (int k = 0; k < num_keys; k++) {
    auto iter = table.get(std::to_string(k));
    for (int v = 0; v < num_values; v++) {
      ASSERT_TRUE(iter->hasNext());
      ASSERT_EQ(num_values - v, iter->available());
      ASSERT_EQ(std::to_string(v), iter->next());
    }
  }
}

TEST_F(MphTableBuilderFixture, PutTenMillionValuesPerKeyThenBuildAndIterate) {
  Options options;
  options.verbose = true;
  const int num_keys = 10;
  const int num_values = mt::MiB(10);
  const Stats stats = buildMphTable(getPrefix(), options, num_keys, num_values);
  ASSERT_GE(stats.key_size_avg, stats.key_size_min);
  ASSERT_LE(stats.key_size_avg, stats.key_size_max);
  ASSERT_EQ(std::to_string(num_keys - 1).size(), stats.key_size_max);
  ASSERT_EQ(1, stats.key_size_min);
  ASSERT_EQ(num_values, stats.list_size_avg);
  ASSERT_EQ(num_values, stats.list_size_max);
  ASSERT_EQ(num_values, stats.list_size_min);
  ASSERT_EQ(num_keys, stats.num_keys_total);
  ASSERT_EQ(num_keys, stats.num_keys_valid);
  ASSERT_EQ(0, stats.num_partitions);
  ASSERT_EQ(num_keys * num_values, stats.num_values_total);
  ASSERT_EQ(num_keys * num_values, stats.num_values_valid);

  MphTable table(getPrefix());
  for (int k = 0; k < num_keys; k++) {
    auto iter = table.get(std::to_string(k));
    for (int v = 0; v < num_values; v++) {
      ASSERT_TRUE(iter->hasNext());
      ASSERT_EQ(num_values - v, iter->available());
      ASSERT_EQ(std::to_string(v), iter->next());
    }
  }
}

#endif

}  // namespace internal
}  // namespace multimap
