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
#include <boost/filesystem/operations.hpp>
#include "gmock/gmock.h"
#include "multimap/Map.hpp"

namespace multimap {

using testing::Eq;

const auto TRUE_PREDICATE = [](const Bytes&) { return true; };
const auto FALSE_PREDICATE = [](const Bytes&) { return false; };
const auto NULL_PROCEDURE = [](const Bytes&) {};
const auto NULL_FUNCTION = [](const Bytes&) { return std::string(); };
const auto NULL_BINARY_PROCEDURE = [](const Bytes&, Map::Iterator&&) {};

TEST(MapTest, IsNotDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<Map>::value);
}

TEST(MapTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<Map>::value);
  ASSERT_FALSE(std::is_copy_assignable<Map>::value);
}

TEST(MapTest, IsNotMoveConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_move_constructible<Map>::value);
  ASSERT_FALSE(std::is_move_assignable<Map>::value);
}

struct MapTestFixture : public testing::Test {
  void SetUp() override {
    directory = "/tmp/multimap.MapTestFixture";
    boost::filesystem::remove_all(directory);
    boost::filesystem::create_directory(directory);
  }

  void TearDown() override { boost::filesystem::remove_all(directory); }

  boost::filesystem::path directory;
};

TEST_F(MapTestFixture, ConstructorThrowsIfFilesAreMissing) {
  Options options;
  options.create_if_missing = false;
  ASSERT_THROW(Map(directory, options), std::runtime_error);
}

TEST_F(MapTestFixture, ConstructorDoesNotThrowIfCreateIsMissingIsTrue) {
  {
    Options options;
    options.create_if_missing = true;
    Map(directory, options);
  }
  ASSERT_FALSE(boost::filesystem::is_empty(directory));
}

TEST_F(MapTestFixture, ConstructorThrowsIfMapExistsAndErrorIfExistsIsTrue) {
  {
    Options options;
    options.create_if_missing = true;
    Map(directory, options);
  }
  Options options;
  options.error_if_exists = true;
  ASSERT_THROW(Map(directory, options), std::runtime_error);
}

TEST_F(MapTestFixture, ConstructorThrowsIfBlockSizeIsNotPowerOfTwo) {
  Options options;
  options.create_if_missing = true;
  options.block_size = 100;
  ASSERT_THROW(Map(directory, options), std::runtime_error);
}

TEST_F(MapTestFixture, PutValueWithMaxKeySize) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);
  try {
    std::string key(Map::Limits::maxKeySize(), 'k');
    ASSERT_NO_THROW(map.put(key, "value"));
  } catch (...) {
    // Allocating `key` may fail.
  }
}

TEST_F(MapTestFixture, PutValueWithTooLargeKeyThrows) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);
  try {
    std::string key(Map::Limits::maxKeySize() + 1, 'k');
    ASSERT_THROW(map.put(key, "value"), std::runtime_error);
  } catch (...) {
    // Allocating `key` may fail.
  }
}

TEST_F(MapTestFixture, PutValueWithMaxValueSize) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);
  try {
    std::string value(Map::Limits::maxValueSize(), 'v');
    ASSERT_NO_THROW(map.put("key", value));
  } catch (...) {
    // Allocating `value` may fail.
  }
}

TEST_F(MapTestFixture, PutValueWithTooLargeValueThrows) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);
  try {
    std::string value(Map::Limits::maxValueSize() + 1, 'v');
    ASSERT_THROW(map.put("key", value), std::runtime_error);
  } catch (...) {
    // Allocating `value` may fail.
  }
}

TEST_F(MapTestFixture, ReadOnlyModeDoesNotAllowUpdates) {
  {
    Options options;
    options.create_if_missing = true;
    Map map(directory, options);
    for (auto k = 0; k != 100; ++k) {
      for (auto v = 0; v != 100; ++v) {
        map.put(std::to_string(k), std::to_string(v));
      }
    }
  }

  Options options;
  options.readonly = true;
  Map map(directory, options);

  const auto key = std::to_string(23);
  ASSERT_NO_THROW(map.forEachEntry(NULL_BINARY_PROCEDURE));
  ASSERT_NO_THROW(map.forEachKey(NULL_PROCEDURE));
  ASSERT_NO_THROW(map.forEachValue(key, NULL_PROCEDURE));
  ASSERT_NO_THROW(map.forEachValue(key, TRUE_PREDICATE));
  ASSERT_NO_THROW(map.get(key));
  ASSERT_NO_THROW(map.getStats());

  const auto value = std::to_string(42);
  ASSERT_THROW(map.put(key, value), std::runtime_error);
  ASSERT_THROW(map.removeKey(key), std::runtime_error);
  ASSERT_THROW(map.removeValues(key, TRUE_PREDICATE), std::runtime_error);
  ASSERT_THROW(map.removeValues(key, Equal(value)), std::runtime_error);
  ASSERT_THROW(map.removeValue(key, TRUE_PREDICATE), std::runtime_error);
  ASSERT_THROW(map.removeValue(key, Equal(value)), std::runtime_error);
  ASSERT_THROW(map.replaceValues(key, NULL_FUNCTION), std::runtime_error);
  ASSERT_THROW(map.replaceValues(key, value, "43"), std::runtime_error);
  ASSERT_THROW(map.replaceValue(key, NULL_FUNCTION), std::runtime_error);
  ASSERT_THROW(map.replaceValue(key, value, "43"), std::runtime_error);
}

struct MapTestWithParam : public testing::TestWithParam<std::int32_t> {
  void SetUp() override {
    directory = "/tmp/multimap.MapTestWithParam";
    boost::filesystem::remove_all(directory);
    MT_ASSERT_TRUE(boost::filesystem::create_directory(directory));
  }

  void TearDown() override {
    MT_ASSERT_TRUE(boost::filesystem::remove_all(directory));
  }

  boost::filesystem::path directory;
};

TEST_P(MapTestWithParam, PutThenGet) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);
  for (std::int32_t k = 0; k != GetParam(); ++k) {
    for (std::int32_t v = 0; v != GetParam(); ++v) {
      map.put(std::to_string(k), std::to_string(v));
    }
  }
  for (std::int32_t k = 0; k != GetParam(); ++k) {
    auto iter = map.get(std::to_string(k));
    ASSERT_THAT(iter.available(), Eq(GetParam()));
    for (std::uint32_t v = 0; iter.hasNext(); ++v) {
      ASSERT_EQ(iter.next().toString(), std::to_string(v));
    }
    ASSERT_EQ(iter.available(), 0);
  }
}

TEST_P(MapTestWithParam, PutThenCloseThenOpenThenGet) {
  {
    Options options;
    options.create_if_missing = true;
    Map map(directory, options);
    for (std::int32_t k = 0; k != GetParam(); ++k) {
      for (std::int32_t v = 0; v != GetParam(); ++v) {
        map.put(std::to_string(k), std::to_string(v));
      }
    }
  }

  Map map(directory, Options());
  for (std::int32_t k = 0; k != GetParam(); ++k) {
    auto iter = map.get(std::to_string(k));
    ASSERT_EQ(iter.available(), GetParam());
    for (std::uint32_t v = 0; iter.hasNext(); ++v) {
      ASSERT_EQ(iter.next().toString(), std::to_string(v));
    }
    ASSERT_EQ(iter.available(), 0);
  }
}

TEST_P(MapTestWithParam, TotalStatsReportsCorrectValuesAfterPut) {
  {
    Options options;
    options.create_if_missing = true;
    Map map(directory, options);
    for (std::int32_t k = 0; k != GetParam(); ++k) {
      for (std::int32_t v = 0; v != GetParam(); ++v) {
        map.put(std::to_string(k), std::to_string(v));
      }
    }

    const auto stats = map.getTotalStats();
    ASSERT_THAT(stats.list_size_avg, Eq(GetParam()));
    ASSERT_THAT(stats.list_size_max, Eq(GetParam()));
    ASSERT_THAT(stats.list_size_min, Eq(GetParam()));
    ASSERT_THAT(stats.num_keys_total, Eq(GetParam()));
    ASSERT_THAT(stats.num_values_total, Eq(GetParam() * GetParam()));
    ASSERT_THAT(stats.num_values_valid, Eq(0));
  }

  Map map(directory, Options());
  const auto stats = map.getTotalStats();
  ASSERT_THAT(stats.list_size_avg, Eq(GetParam()));
  ASSERT_THAT(stats.list_size_max, Eq(GetParam()));
  ASSERT_THAT(stats.list_size_min, Eq(GetParam()));
  ASSERT_THAT(stats.num_keys_total, Eq(GetParam()));
  ASSERT_THAT(stats.num_values_total, Eq(GetParam() * GetParam()));
  ASSERT_THAT(stats.num_values_valid, Eq(0));
}

// TEST_P(MapTestWithParam, TotalStatsAreCorrectAfterRemovingValues) {
//  Map::Stats stats_backup;
//  std::size_t num_removed = 0;

//  const auto is_prime = [](const Bytes& value) {
//    const auto str = value.toString();
//    return mt::isPrime(std::stoul(str));
//  };

//  {
//    Options options;
//    options.create_if_missing = true;
//    Map map(directory, options);
//    for (std::int32_t k = 0; k != GetParam(); ++k) {
//      for (std::int32_t v = 0; v != GetParam(); ++v) {
//        map.put(std::to_string(k), std::to_string(v));
//      }
//    }

//    num_removed += map.removeAll("0", is_prime);
////    num_removed += map.removeAll("1", is_prime);
////    num_removed += map.removeAll("2", is_prime);
////    num_removed += map.removeAll("3", is_prime);
////    num_removed += map.removeAll("4", is_prime);

////    num_removed += map.remove("5");
////    num_removed += map.remove("6");
////    num_removed += map.remove("7");
////    num_removed += map.remove("8");
////    num_removed += map.remove("9");

//    const auto stats = map.getTotalStats();
//    ASSERT_THAT(stats.num_keys, Eq(GetParam()));
//    ASSERT_THAT(stats.num_values_put, Eq(GetParam() * GetParam()));
//    ASSERT_THAT(stats.num_values_removed, Eq(num_removed));
//    stats_backup = stats;
//  }

//  {
//    Map map(directory, Options());
//    auto stats = map.getTotalStats();
//    ASSERT_THAT(stats.num_keys, Eq(GetParam()));
//    ASSERT_THAT(stats.num_values_put, Eq(stats_backup.num_values_put));
//    ASSERT_THAT(stats.num_values_removed,
//    Eq(stats_backup.num_values_removed));

//    for (std::int32_t k = 0; k != GetParam(); ++k) {
//      for (std::int32_t v = 0; v != GetParam(); ++v) {
//        map.put(std::to_string(k), std::to_string(v));
//      }
//    }

//    num_removed += map.removeAll("0", is_prime);
////    num_removed += map.removeAll("1", is_prime);
////    num_removed += map.removeAll("2", is_prime);
////    num_removed += map.removeAll("3", is_prime);
////    num_removed += map.removeAll("4", is_prime);

////    num_removed += map.remove("5");
////    num_removed += map.remove("6");
////    num_removed += map.remove("7");
////    num_removed += map.remove("8");
////    num_removed += map.remove("9");

//    stats = map.getTotalStats();
//    ASSERT_THAT(stats.num_values_put, Eq(GetParam() * GetParam() * 2));
//    ASSERT_THAT(stats.num_values_removed, Eq(num_removed));
//  }
//}

INSTANTIATE_TEST_CASE_P(Parameterized, MapTestWithParam,
                        testing::Values(0, 1, 2, 10, 100, 1000));

// INSTANTIATE_TEST_CASE_P(ParameterizedLongRunning, MapTestWithParam,
//                         testing::Values(10000, 100000));

TEST_F(MapTestFixture, RemoveKeyReturnsTrueForExistingKey) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);
  map.put("key", "value");
  ASSERT_TRUE(map.removeKey("key"));
}

TEST_F(MapTestFixture, RemoveKeyReturnsFalseForNonExistingKey) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);
  ASSERT_FALSE(map.removeKey("key"));
}

TEST_F(MapTestFixture, RemoveValueRemovesOnlyFirstMatch) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);

  const std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.put("key", std::to_string(i));
    map.put("key", std::to_string(i));
  }

  ASSERT_FALSE(map.removeValue("key", Equal("500")));
  ASSERT_EQ(map.get("key").available(), num_values);

  ASSERT_TRUE(map.removeValue("key", Equal("250")));
  ASSERT_EQ(map.get("key").available(), num_values - 1);
}

TEST_F(MapTestFixture, RemoveValuesRemovesAllMatches) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);

  const std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.put("key", std::to_string(i));
    map.put("key", std::to_string(i));
  }

  ASSERT_EQ(map.removeValues("key", Equal("500")), 0);
  ASSERT_EQ(map.get("key").available(), num_values);

  ASSERT_EQ(map.removeValues("key", Equal("250")), 2);
  ASSERT_EQ(map.get("key").available(), num_values - 2);
}

TEST_F(MapTestFixture, ReplaceValueReplacesOnlyFirstMatch) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);

  const std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.put("key", std::to_string(i));
    map.put("key", std::to_string(i));
  }

  ASSERT_FALSE(map.replaceValue("key", "500", "2500"));
  ASSERT_EQ(map.get("key").available(), num_values);

  ASSERT_TRUE(map.replaceValue("key", "250", "2500"));
  ASSERT_EQ(map.get("key").available(), num_values);

  // Check if the "2500" replacement has been added to the end of the list.
  auto iter = map.get("key");
  for (std::size_t i = 0; i != 250; ++i) {
    ASSERT_EQ(iter.next().toString(), std::to_string(i));
    ASSERT_EQ(iter.next().toString(), std::to_string(i));
  }
  ASSERT_EQ(iter.next().toString(), "250");
  // Only one "250" value is left here.
  for (std::size_t i = 251; i != num_values / 2; ++i) {
    ASSERT_EQ(iter.next().toString(), std::to_string(i));
    ASSERT_EQ(iter.next().toString(), std::to_string(i));
  }
  ASSERT_EQ(iter.next().toString(), "2500");
  ASSERT_EQ(iter.available(), 0);
}

TEST_F(MapTestFixture, ReplaceValueApplyingMapReplacesOnlyFirstMatch) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);

  const std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.put("key", std::to_string(i));
    map.put("key", std::to_string(i));
  }

  ASSERT_FALSE(map.replaceValue("key", NULL_FUNCTION));
  ASSERT_EQ(map.get("key").available(), num_values);

  const auto map_250_to_2500 = [](const Bytes& value) -> std::string {
    return (value.toString() == "250") ? "2500" : "";
  };
  ASSERT_TRUE(map.replaceValue("key", map_250_to_2500));
  ASSERT_EQ(map.get("key").available(), num_values);

  // Check if the "2500" replacement has been added to the end of the list.
  auto iter = map.get("key");
  for (std::size_t i = 0; i != 250; ++i) {
    ASSERT_EQ(iter.next().toString(), std::to_string(i));
    ASSERT_EQ(iter.next().toString(), std::to_string(i));
  }
  ASSERT_EQ(iter.next().toString(), "250");
  // Only one "250" value is left here.
  for (std::size_t i = 251; i != num_values / 2; ++i) {
    ASSERT_EQ(iter.next().toString(), std::to_string(i));
    ASSERT_EQ(iter.next().toString(), std::to_string(i));
  }
  ASSERT_EQ(iter.next().toString(), "2500");
  ASSERT_EQ(iter.available(), 0);
}

TEST_F(MapTestFixture, ReplaceValuesReplacesAllMatches) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);

  const std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.put("key", std::to_string(i));
    map.put("key", std::to_string(i));
  }

  ASSERT_EQ(map.replaceValues("key", "500", "2500"), 0);
  ASSERT_EQ(map.get("key").available(), num_values);

  ASSERT_EQ(map.replaceValues("key", "250", "2500"), 2);
  ASSERT_EQ(map.get("key").available(), num_values);

  // Check if both "2500" replacements have been added to the end of the list.
  auto iter = map.get("key");
  for (std::size_t i = 0; i != 250; ++i) {
    ASSERT_EQ(iter.next().toString(), std::to_string(i));
    ASSERT_EQ(iter.next().toString(), std::to_string(i));
  }
  for (std::size_t i = 251; i != num_values / 2; ++i) {
    ASSERT_EQ(iter.next().toString(), std::to_string(i));
    ASSERT_EQ(iter.next().toString(), std::to_string(i));
  }
  ASSERT_EQ(iter.next().toString(), "2500");
  ASSERT_EQ(iter.next().toString(), "2500");
  ASSERT_EQ(iter.available(), 0);
}

TEST_F(MapTestFixture, ReplaceValuesApplyingMapReplacesAllMatches) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);

  const std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.put("key", std::to_string(i));
    map.put("key", std::to_string(i));
  }

  ASSERT_EQ(map.replaceValues("key", NULL_FUNCTION), 0);
  ASSERT_EQ(map.get("key").available(), num_values);

  const auto map_250_to_2500 = [](const Bytes& value) -> std::string {
    return (value.toString() == "250") ? "2500" : "";
  };
  ASSERT_EQ(map.replaceValues("key", map_250_to_2500), 2);
  ASSERT_EQ(map.get("key").available(), num_values);

  // Check if both "2500" replacements have been added to the end of the list.
  auto iter = map.get("key");
  for (std::size_t i = 0; i != 250; ++i) {
    ASSERT_EQ(iter.next().toString(), std::to_string(i));
    ASSERT_EQ(iter.next().toString(), std::to_string(i));
  }
  for (std::size_t i = 251; i != num_values / 2; ++i) {
    ASSERT_EQ(iter.next().toString(), std::to_string(i));
    ASSERT_EQ(iter.next().toString(), std::to_string(i));
  }
  ASSERT_EQ(iter.next().toString(), "2500");
  ASSERT_EQ(iter.next().toString(), "2500");
  ASSERT_EQ(iter.available(), 0);
}

// Move that into ShardTest
//TEST_F(MapTestFixture, IteratorWritesBackMutatedBlocks) {
//  Options options;
//  options.create_if_missing = true;
//  Map map(directory, options);

//  const std::size_t num_values = 100000;
//  for (std::size_t i = 0; i != num_values; ++i) {
//    map.put("key", "value" + std::to_string(i));
//  }

//  {
//    auto iter = map.getMutable("key");
//    ASSERT_EQ(iter.available(), num_values);
//    for (std::size_t i = 0; i != num_values / 2; ++i) {
//      if (i == 113) {
//        ASSERT_TRUE(iter.hasNext());
//        ASSERT_EQ(iter.next().toString(), "value" + std::to_string(i));
//      } else {
//        ASSERT_TRUE(iter.hasNext());
//        ASSERT_EQ(iter.next().toString(), "value" + std::to_string(i));
//      }
//      iter.remove();
//    }
//  }

//  auto iter = map.get("key");
//  ASSERT_EQ(iter.available(), num_values / 2);
//  for (std::size_t i = num_values / 2; i != num_values; ++i) {
//    ASSERT_TRUE(iter.hasNext());
//    ASSERT_EQ(iter.next().toString(), "value" + std::to_string(i));
//  }
//}

}  // namespace multimap
