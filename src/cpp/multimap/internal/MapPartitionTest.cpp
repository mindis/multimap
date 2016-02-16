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
#include "multimap/internal/Table.hpp"
#include "multimap/callables.hpp"

namespace multimap {
namespace internal {

using testing::Eq;
using testing::ElementsAre;
using testing::UnorderedElementsAre;

std::unique_ptr<Table> openTable(const boost::filesystem::path& prefix,
                                 const Table::Options& options) {
  return std::unique_ptr<Table>(new Table(prefix, options));
}

std::unique_ptr<Table> openOrCreateTable(
    const boost::filesystem::path& prefix) {
  Table::Options options;
  options.create_if_missing = true;
  return std::unique_ptr<Table>(new Table(prefix, options));
}

std::unique_ptr<Table> openOrCreateTableAsReadOnly(
    const boost::filesystem::path& prefix) {
  Table::Options options;
  options.readonly = true;
  options.create_if_missing = true;
  return std::unique_ptr<Table>(new Table(prefix, options));
}

TEST(TableTest, IsNotDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<Table>::value);
}

TEST(TableTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<Table>::value);
  ASSERT_FALSE(std::is_copy_assignable<Table>::value);
}

TEST(TableTest, IsNotMoveConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_move_constructible<Table>::value);
  ASSERT_FALSE(std::is_move_assignable<Table>::value);
}

struct TableTestFixture : public testing::Test {
  void SetUp() override {
    boost::filesystem::remove_all(directory);
    boost::filesystem::create_directory(directory);
  }

  void TearDown() override { boost::filesystem::remove_all(directory); }

  const boost::filesystem::path directory = "/tmp/multimap.TableTestFixture";
  const boost::filesystem::path prefix = directory / "table";
  const std::string k1 = "k1";
  const std::string k2 = "k2";
  const std::string k3 = "k3";
  const std::string v1 = "v1";
  const std::string v2 = "v2";
  const std::string v3 = "v3";
};

TEST_F(TableTestFixture, ConstructorThrowsIfNotExist) {
  ASSERT_THROW(Table(this->prefix), std::runtime_error);
  // GCC complains when not using 'this' pointer.
}

TEST_F(TableTestFixture, ConstructorWithDefaultOptionsThrowsIfNotExist) {
  Table::Options options;
  ASSERT_THROW(Table(prefix, options), std::runtime_error);
}

TEST_F(TableTestFixture, ConstructorWithCreateIfMissingDoesNotThrow) {
  Table::Options options;
  options.create_if_missing = true;
  ASSERT_NO_THROW(Table(prefix, options));
}

TEST_F(TableTestFixture, PutAppendsValuesToList) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  table->put(k1, v2);
  table->put(k1, v3);
  auto iter = table->get(k1);
  ASSERT_THAT(iter.next(), Eq(v1));
  ASSERT_THAT(iter.next(), Eq(v2));
  ASSERT_THAT(iter.next(), Eq(v3));
  ASSERT_FALSE(iter.hasNext());
}

TEST_F(TableTestFixture, PutMaxKeyDoesNotThrow) {
  auto table = openOrCreateTable(prefix);
  try {
    std::string key(Table::Limits::maxKeySize(), 'k');
    ASSERT_NO_THROW(table->put(key, v1));
  } catch (...) {
    // Allocating key itself may fail.
  }
}

TEST_F(TableTestFixture, PutTooBigKeyThrows) {
  auto table = openOrCreateTable(prefix);
  try {
    std::string key(Table::Limits::maxKeySize() + 1, 'k');
    ASSERT_THROW(table->put(key, v1), mt::AssertionError);
  } catch (...) {
    // Allocating key itself may fail.
  }
}

TEST_F(TableTestFixture, PutMaxValueDoesNotThrow) {
  auto table = openOrCreateTable(prefix);
  try {
    std::string value(Table::Limits::maxValueSize(), 'v');
    ASSERT_NO_THROW(table->put(k1, value));
  } catch (...) {
    // Allocating value itself may fail.
  }
}

TEST_F(TableTestFixture, PutTooBigValueThrows) {
  auto table = openOrCreateTable(prefix);
  try {
    std::string value(Table::Limits::maxValueSize() + 1, 'v');
    ASSERT_THROW(table->put(k1, value), mt::AssertionError);
  } catch (...) {
    // Allocating value itself may fail.
  }
}

TEST_F(TableTestFixture, PutValuesAndReopenInBetween) {
  {
    Table::Options options;
    options.block_size = 128;
    options.create_if_missing = true;
    auto table = openTable(prefix, options);
    table->put(k1, v1);
    table->put(k2, v1);
    table->put(k3, v1);
  }
  {
    auto table = openOrCreateTable(prefix);
    table->put(k1, v2);
    table->put(k2, v2);
    table->put(k3, v2);
  }
  {
    auto table = openOrCreateTable(prefix);
    table->put(k1, v3);
    table->put(k2, v3);
    table->put(k3, v3);
  }
  auto table = openOrCreateTable(prefix);
  auto iter1 = table->get(k1);
  ASSERT_THAT(iter1.next(), Eq(v1));
  ASSERT_THAT(iter1.next(), Eq(v2));
  ASSERT_THAT(iter1.next(), Eq(v3));
  ASSERT_FALSE(iter1.hasNext());

  auto iter2 = table->get(k2);
  ASSERT_THAT(iter2.next(), Eq(v1));
  ASSERT_THAT(iter2.next(), Eq(v2));
  ASSERT_THAT(iter2.next(), Eq(v3));
  ASSERT_FALSE(iter2.hasNext());

  auto iter3 = table->get(k3);
  ASSERT_THAT(iter3.next(), Eq(v1));
  ASSERT_THAT(iter3.next(), Eq(v2));
  ASSERT_THAT(iter3.next(), Eq(v3));
  ASSERT_FALSE(iter3.hasNext());
}

TEST_F(TableTestFixture, GetReturnsEmptyListForNonExistingKey) {
  auto table = openOrCreateTable(prefix);
  ASSERT_FALSE(table->get(k1).hasNext());
}

TEST_F(TableTestFixture, RemoveKeyRemovesMappedValues) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  table->put(k2, v1);
  table->put(k3, v1);
  ASSERT_TRUE(table->removeKey(k1));
  ASSERT_TRUE(table->removeKey(k2));
  ASSERT_FALSE(table->get(k1).hasNext());
  ASSERT_FALSE(table->get(k2).hasNext());
  ASSERT_TRUE(table->get(k3).hasNext());
}

TEST_F(TableTestFixture, RemoveKeysRemovesMappedValues) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  table->put(k2, v1);
  table->put(k3, v1);
  auto is_k1_or_k2 = [&](const Bytes& key) { return key == k1 || key == k2; };
  ASSERT_THAT(table->removeKeys(is_k1_or_k2), Eq(2));
  ASSERT_FALSE(table->get(k1).hasNext());
  ASSERT_FALSE(table->get(k2).hasNext());
  ASSERT_TRUE(table->get(k3).hasNext());
}

TEST_F(TableTestFixture, RemoveValueRemovesOnlyFirstMatch) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  table->put(k1, v2);
  table->put(k1, v3);
  auto is_any_value = [&](const Bytes& /* value */) { return true; };
  ASSERT_TRUE(table->removeValue(k1, is_any_value));
  auto iter = table->get(k1);
  ASSERT_THAT(iter.next(), Eq(v2));
  ASSERT_THAT(iter.next(), Eq(v3));
  ASSERT_FALSE(iter.hasNext());

  table->put(k2, v1);
  table->put(k2, v2);
  table->put(k2, v3);
  auto is_v2_or_v3 =
      [&](const Bytes& value) { return value == v2 || value == v3; };
  ASSERT_TRUE(table->removeValue(k2, is_v2_or_v3));
  iter = table->get(k2);
  ASSERT_THAT(iter.next(), Eq(v1));
  ASSERT_THAT(iter.next(), Eq(v3));
  ASSERT_FALSE(iter.hasNext());
}

TEST_F(TableTestFixture, RemoveValuesRemovesAllMatches) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  table->put(k1, v2);
  table->put(k1, v3);
  auto is_any_value = [&](const Bytes& /* value */) { return true; };
  ASSERT_THAT(table->removeValues(k1, is_any_value), Eq(3));
  auto iter = table->get(k1);
  ASSERT_FALSE(iter.hasNext());

  table->put(k2, v1);
  table->put(k2, v2);
  table->put(k2, v3);
  auto is_v2_or_v3 =
      [&](const Bytes& value) { return value == v2 || value == v3; };
  ASSERT_TRUE(table->removeValues(k2, is_v2_or_v3));
  iter = table->get(k2);
  ASSERT_THAT(iter.next(), Eq(v1));
  ASSERT_FALSE(iter.hasNext());
}

TEST_F(TableTestFixture, ReplaceValueReplacesOnlyFirstMatch) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  table->put(k1, v2);
  table->put(k1, v3);
  auto rotate_any = [&](const Bytes& value) {
    if (value == v1)
      return v2;
    if (value == v2)
      return v3;
    if (value == v3)
      return v1;
    return std::string();
  };
  ASSERT_TRUE(table->replaceValue(k1, rotate_any));
  auto iter = table->get(k1);
  ASSERT_THAT(iter.next(), Eq(v2));
  ASSERT_THAT(iter.next(), Eq(v3));
  ASSERT_THAT(iter.next(), Eq(v2)); // v1 replacement
  ASSERT_FALSE(iter.hasNext());

  table->put(k2, v1);
  table->put(k2, v2);
  table->put(k2, v3);
  auto rotate_v2_or_v3 = [&](const Bytes& value) {
    if (value == v2)
      return v3;
    if (value == v3)
      return v1;
    return std::string();
  };
  ASSERT_TRUE(table->replaceValue(k2, rotate_v2_or_v3));
  iter = table->get(k2);
  ASSERT_THAT(iter.next(), Eq(v1));
  ASSERT_THAT(iter.next(), Eq(v3));
  ASSERT_THAT(iter.next(), Eq(v3)); // v2 replacement
  ASSERT_FALSE(iter.hasNext());
}

TEST_F(TableTestFixture, ReplaceValuesReplacesAllMatches) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  table->put(k1, v2);
  table->put(k1, v3);
  auto rotate_any = [&](const Bytes& value) {
    if (value == v1)
      return v2;
    if (value == v2)
      return v3;
    if (value == v3)
      return v1;
    return std::string();
  };
  ASSERT_THAT(table->replaceValues(k1, rotate_any), Eq(3));
  auto iter = table->get(k1);
  ASSERT_THAT(iter.next(), Eq(v2)); // v1 replacement
  ASSERT_THAT(iter.next(), Eq(v3)); // v2 replacement
  ASSERT_THAT(iter.next(), Eq(v1)); // v3 replacement
  ASSERT_FALSE(iter.hasNext());

  table->put(k2, v1);
  table->put(k2, v2);
  table->put(k2, v3);
  auto rotate_v2_or_v3 = [&](const Bytes& value) {
    if (value == v2)
      return v3;
    if (value == v3)
      return v1;
    return std::string();
  };
  ASSERT_THAT(table->replaceValues(k2, rotate_v2_or_v3), Eq(2));
  iter = table->get(k2);
  ASSERT_THAT(iter.next(), Eq(v1));
  ASSERT_THAT(iter.next(), Eq(v3)); // v2 replacement
  ASSERT_THAT(iter.next(), Eq(v1)); // v3 replacement
  ASSERT_FALSE(iter.hasNext());
}

TEST_F(TableTestFixture, ForEachKeyIgnoresEmptyLists) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  table->put(k2, v1);
  table->put(k3, v1);
  std::vector<std::string> keys;
  auto collect = [&](const Bytes& key) { keys.push_back(key.toString()); };
  table->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k1, k2, k3));

  keys.clear();
  table->removeKey(k1);
  table->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k2, k3));

  keys.clear();
  table->removeKey(k2);
  table->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k3));

  keys.clear();
  table->removeKey(k3);
  table->forEachKey(collect);
  ASSERT_TRUE(keys.empty());

  table->put(k1, v1);
  table->put(k2, v1);
  table->put(k3, v1);
  table->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k1, k2, k3));

  auto any_value = [](const Bytes& /* value */) { return true; };

  keys.clear();
  table->removeValues(k1, any_value);
  table->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k2, k3));

  keys.clear();
  table->removeValues(k2, any_value);
  table->forEachKey(collect);
  ASSERT_THAT(keys, UnorderedElementsAre(k3));

  keys.clear();
  table->removeValues(k3, any_value);
  table->forEachKey(collect);
  ASSERT_TRUE(keys.empty());
}

TEST_F(TableTestFixture, ForEachValueVisitsAllValues) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  table->put(k2, v1);
  table->put(k2, v2);
  std::vector<std::string> values;
  auto collect = [&](const Bytes& val) { values.push_back(val.toString()); };
  table->forEachValue(k1, collect);
  ASSERT_THAT(values, UnorderedElementsAre(v1));

  values.clear();
  table->forEachValue(k2, collect);
  ASSERT_THAT(values, UnorderedElementsAre(v1, v2));

  values.clear();
  table->forEachValue(k3, collect);
  ASSERT_TRUE(values.empty());
}

TEST_F(TableTestFixture, ForEachEntryIgnoresEmptyLists) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  table->put(k2, v1);
  table->put(k2, v2);
  table->put(k3, v1);
  table->put(k3, v2);
  table->put(k3, v3);
  std::map<std::string, std::vector<std::string> > mapping;
  auto collect = [&](const Bytes& key, SharedListIterator&& iter) {
    while (iter.hasNext()) {
      mapping[key.toString()].push_back(iter.next().toString());
    }
  };
  table->forEachEntry(collect);
  ASSERT_THAT(mapping.size(), Eq(3));
  ASSERT_THAT(mapping.at(k1), ElementsAre(v1));
  ASSERT_THAT(mapping.at(k2), ElementsAre(v1, v2));
  ASSERT_THAT(mapping.at(k3), ElementsAre(v1, v2, v3));

  mapping.clear();
  table->removeValues(k2, [](const Bytes& /* value */) { return true; });
  table->forEachEntry(collect);
  ASSERT_THAT(mapping.size(), Eq(2));
  ASSERT_THAT(mapping.at(k1), ElementsAre(v1));
  ASSERT_THAT(mapping.at(k3), ElementsAre(v1, v2, v3));
}

TEST_F(TableTestFixture, GetStatsReturnsCorrectValues) {
  auto table = openOrCreateTable(prefix);
  table->put("k", "vvvvv");
  table->put("kk", "vvvv");
  table->put("kk", "vvvv");
  table->put("kkk", "vvv");
  table->put("kkk", "vvv");
  table->put("kkk", "vvv");
  table->put("kkkk", "vv");
  table->put("kkkk", "vv");
  table->put("kkkk", "vv");
  table->put("kkkk", "vv");
  table->put("kkkkk", "v");
  table->put("kkkkk", "v");
  table->put("kkkkk", "v");
  table->put("kkkkk", "v");
  table->put("kkkkk", "v");

  auto stats = table->getStats();
  ASSERT_THAT(stats.block_size, Eq(Table::Options().block_size));
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

  table->removeKey("kkkkk");
  stats = table->getStats();
  ASSERT_THAT(stats.block_size, Eq(Table::Options().block_size));
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

TEST_F(TableTestFixture, GetStatsReturnsCorrectValuesAfterRemovingKeys) {
  {
    auto table = openOrCreateTable(prefix);
    table->put("k", "vvvvv");
    table->put("kk", "vvvv");
    table->put("kk", "vvvv");
    table->put("kkk", "vvv");
    table->put("kkk", "vvv");
    table->put("kkk", "vvv");
    table->put("kkkk", "vv");
    table->put("kkkk", "vv");
    table->put("kkkk", "vv");
    table->put("kkkk", "vv");
    table->put("kkkkk", "v");
    table->put("kkkkk", "v");
    table->put("kkkkk", "v");
    table->put("kkkkk", "v");
    table->put("kkkkk", "v");

    table->removeKey("k");
    table->removeKeys([](const Bytes& key) { return key == "kk"; });

    const auto stats = table->getStats();
    ASSERT_THAT(stats.num_keys_total, Eq(5));
    ASSERT_THAT(stats.num_keys_valid, Eq(3));
    ASSERT_THAT(stats.num_values_total, Eq(15));
    ASSERT_THAT(stats.num_values_valid, Eq(12));
  }

  auto table = openOrCreateTable(prefix);
  const auto stats = table->getStats();
  ASSERT_THAT(stats.num_keys_total, Eq(3));
  ASSERT_THAT(stats.num_keys_valid, Eq(3));
  ASSERT_THAT(stats.num_values_total, Eq(15));
  ASSERT_THAT(stats.num_values_valid, Eq(12));
}

TEST_F(TableTestFixture, GetStatsReturnsCorrectValuesAfterRemovingValues) {
  {
    auto table = openOrCreateTable(prefix);
    table->put("k", "vvvvv");
    table->put("kk", "vvvv");
    table->put("kk", "vvvv");
    table->put("kkk", "vvv");
    table->put("kkk", "vvv");
    table->put("kkk", "vvv");
    table->put("kkkk", "vv");
    table->put("kkkk", "vv");
    table->put("kkkk", "vv");
    table->put("kkkk", "vv");
    table->put("kkkkk", "v");
    table->put("kkkkk", "v");
    table->put("kkkkk", "v");
    table->put("kkkkk", "v");
    table->put("kkkkk", "v");

    table->removeValue("k", Equal("vvvvv"));
    table->removeValues("kk", [](const Bytes& val) { return val == "vvvv"; });

    const auto stats = table->getStats();
    ASSERT_THAT(stats.num_keys_total, Eq(5));
    ASSERT_THAT(stats.num_keys_valid, Eq(3));
    ASSERT_THAT(stats.num_values_total, Eq(15));
    ASSERT_THAT(stats.num_values_valid, Eq(12));
  }

  auto table = openOrCreateTable(prefix);
  const auto stats = table->getStats();
  ASSERT_THAT(stats.num_keys_total, Eq(3));
  ASSERT_THAT(stats.num_keys_valid, Eq(3));
  ASSERT_THAT(stats.num_values_total, Eq(15));
  ASSERT_THAT(stats.num_values_valid, Eq(12));
}

TEST_F(TableTestFixture, IsReadOnlyReturnsCorrectValue) {
  {
    auto table = openOrCreateTable(prefix);
    ASSERT_FALSE(table->isReadOnly());
  }
  {
    auto table = openOrCreateTableAsReadOnly(prefix);
    ASSERT_TRUE(table->isReadOnly());
  }
}

TEST_F(TableTestFixture, PutThrowsIfOpenedAsReadOnly) {
  auto table = openOrCreateTableAsReadOnly(prefix);
  ASSERT_THROW(table->put(k1, v1), std::runtime_error);
}

TEST_F(TableTestFixture, RemoveKeyThrowsIfOpenedAsReadOnly) {
  auto table = openOrCreateTableAsReadOnly(prefix);
  ASSERT_THROW(table->removeKey(k1), std::runtime_error);
}

TEST_F(TableTestFixture, RemoveKeysThrowsIfOpenedAsReadOnly) {
  auto table = openOrCreateTableAsReadOnly(prefix);
  ASSERT_THROW(table->removeKeys([](const Bytes& /* key */) { return true; }),
               std::runtime_error);
}

TEST_F(TableTestFixture, RemoveValueThrowsIfOpenedAsReadOnly) {
  auto table = openOrCreateTableAsReadOnly(prefix);
  ASSERT_THROW(table->removeValue(k1, Equal(v1)), std::runtime_error);
}

TEST_F(TableTestFixture, RemoveValuesThrowsIfOpenedAsReadOnly) {
  auto table = openOrCreateTableAsReadOnly(prefix);
  ASSERT_THROW(table->removeValues(k1, Equal(v1)), std::runtime_error);
}

TEST_F(TableTestFixture, ReplaceValueThrowsIfOpenedAsReadOnly) {
  auto table = openOrCreateTableAsReadOnly(prefix);
  ASSERT_THROW(table->replaceValue(k1, v1, v2), std::runtime_error);
}

TEST_F(TableTestFixture, ReplaceValuesThrowsIfOpenedAsReadOnly) {
  auto table = openOrCreateTableAsReadOnly(prefix);
  ASSERT_THROW(table->replaceValues(k1, v1, v2), std::runtime_error);
}

TEST_F(TableTestFixture, GetBlockSizeReturnsCorrectValue) {
  auto table = openOrCreateTable(prefix);
  ASSERT_THAT(table->getBlockSize(), Eq(Table::Options().block_size));
}

// -----------------------------------------------------------------------------
// class Table / Mutability
// -----------------------------------------------------------------------------

struct TableTestWithParam : public testing::TestWithParam<int> {
  void SetUp() override {
    boost::filesystem::remove_all(directory);
    boost::filesystem::create_directory(directory);
  }

  void TearDown() override { boost::filesystem::remove_all(directory); }

  const boost::filesystem::path directory = "/tmp/multimap.TableTestWithParam";
  const boost::filesystem::path prefix = directory / "table";
};

TEST_P(TableTestWithParam, PutDataThenReadAll) {
  {
    auto table = openOrCreateTable(prefix);
    for (auto k = 0; k != GetParam(); ++k) {
      for (auto v = 0; v != GetParam(); ++v) {
        table->put(std::to_string(k), std::to_string(v));
      }
    }

    for (auto k = 0; k != GetParam(); ++k) {
      auto iter = table->get(std::to_string(k));
      for (auto v = 0; v != GetParam(); ++v) {
        ASSERT_TRUE(iter.hasNext());
        ASSERT_THAT(iter.next(), std::to_string(v));
      }
      ASSERT_FALSE(iter.hasNext());
    }
  }

  auto table = openOrCreateTable(prefix);
  for (auto k = 0; k != GetParam(); ++k) {
    auto iter = table->get(std::to_string(k));
    for (auto v = 0; v != GetParam(); ++v) {
      ASSERT_TRUE(iter.hasNext());
      ASSERT_THAT(iter.next(), std::to_string(v));
    }
    ASSERT_FALSE(iter.hasNext());
  }
}

TEST_P(TableTestWithParam, PutDataThenRemoveSomeThenReadAll) {
  auto is_odd = [](const Bytes& b) { return std::stoul(b.toString()) % 2; };
  {
    auto table = openOrCreateTable(prefix);
    for (auto k = 0; k != GetParam(); ++k) {
      for (auto v = 0; v != GetParam(); ++v) {
        table->put(std::to_string(k), std::to_string(v));
      }
    }

    for (auto k = 0; k != GetParam(); ++k) {
      const auto key = std::to_string(k);
      if (is_odd(key)) {
        table->removeValues(key, is_odd);
      }
    }

    for (auto k = 0; k != GetParam(); ++k) {
      const auto key = std::to_string(k);
      auto iter = table->get(key);
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

  auto table = openOrCreateTable(prefix);
  for (auto k = 0; k != GetParam(); ++k) {
    const auto key = std::to_string(k);
    auto iter = table->get(key);
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

INSTANTIATE_TEST_CASE_P(Parameterized, TableTestWithParam,
                        testing::Values(0, 1, 2, 10, 100, 1000));

// INSTANTIATE_TEST_CASE_P(ParameterizedLongRunning, TableTestWithParam,
//                         testing::Values(10000, 100000));

// -----------------------------------------------------------------------------
// class Table / Concurrency
// -----------------------------------------------------------------------------

const auto NULL_PROCEDURE = [](const Bytes&) {};
const auto TRUE_PREDICATE = [](const Bytes&) { return true; };
const auto EMPTY_FUNCTION = [](const Bytes&) { return std::string(); };

TEST_F(TableTestFixture, GetDifferentListsDoesNotBlock) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  table->put(k2, v1);
  auto iter1 = table->get(k1);
  ASSERT_TRUE(iter1.hasNext());
  auto iter2 = table->get(k2);
  ASSERT_TRUE(iter2.hasNext());
}

TEST_F(TableTestFixture, GetSameListTwiceDoesNotBlock) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  auto iter1 = table->get(k1);
  ASSERT_TRUE(iter1.hasNext());
  auto iter2 = table->get(k1);
  ASSERT_TRUE(iter2.hasNext());
}

TEST_F(TableTestFixture, RemoveKeyBlocksIfListIsLocked) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  auto thread_is_running = true;
  {
    auto iter = table->get(k1);
    ASSERT_TRUE(iter.hasNext());
    std::thread thread([&] {
      table->removeKey(k1);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(TableTestFixture, RemoveKeysBlocksIfListIsLocked) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  auto thread_is_running = true;
  {
    auto iter = table->get(k1);
    ASSERT_TRUE(iter.hasNext());
    std::thread thread([&] {
      table->removeKeys(TRUE_PREDICATE);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(TableTestFixture, RemoveValueBlocksIfListIsLocked) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  auto thread_is_running = true;
  {
    auto iter = table->get(k1);
    ASSERT_TRUE(iter.hasNext());
    std::thread thread([&] {
      table->removeValue(k1, TRUE_PREDICATE);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(TableTestFixture, RemoveValuesBlocksIfListIsLocked) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  auto thread_is_running = true;
  {
    auto iter = table->get(k1);
    ASSERT_TRUE(iter.hasNext());
    std::thread thread([&] {
      table->removeValues(k1, TRUE_PREDICATE);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(TableTestFixture, ReplaceValueBlocksIfListIsLocked) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  auto thread_is_running = true;
  {
    auto iter = table->get(k1);
    ASSERT_TRUE(iter.hasNext());
    std::thread thread([&] {
      table->replaceValue(k1, EMPTY_FUNCTION);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(TableTestFixture, ReplaceValuesBlocksIfListIsLocked) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  auto thread_is_running = true;
  {
    auto iter = table->get(k1);
    ASSERT_TRUE(iter.hasNext());
    std::thread thread([&] {
      table->replaceValues(k1, EMPTY_FUNCTION);
      thread_is_running = false;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(thread_is_running);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(TableTestFixture, ForEachKeyDoesNotBlockIfListIsLocked) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  auto thread_is_running = true;
  std::thread thread([&] {
    table->forEachKey(NULL_PROCEDURE);
    thread_is_running = false;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(TableTestFixture, ForEachValueDoesNotBlockIfListIsLocked) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  auto thread_is_running = true;
  std::thread thread([&] {
    table->forEachValue(k1, NULL_PROCEDURE);
    thread_is_running = false;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

TEST_F(TableTestFixture, ForEachEntryDoesNotBlockIfListIsLocked) {
  auto table = openOrCreateTable(prefix);
  table->put(k1, v1);
  auto thread_is_running = true;
  std::thread thread([&] {
    table->forEachEntry([](const Bytes&, Table::Iterator&&) {});
    thread_is_running = false;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(thread_is_running);
}

// -----------------------------------------------------------------------------
// class Table::Stats
// -----------------------------------------------------------------------------

TEST(TableStatsTest, NamesAndToVectorHaveSameDimension) {
  const auto names = Table::Stats::names();
  const auto vector = Table::Stats().toVector();
  ASSERT_THAT(names.size(), Eq(vector.size()));
}

} // namespace internal
} // namespace multimap
