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

struct MapTestFixture : public testing::Test {
  void SetUp() override {
    directory = "/tmp/multimap-MapTestFixture";
    boost::filesystem::remove_all(directory);
    assert(boost::filesystem::create_directory(directory));
  }

  void TearDown() override { assert(boost::filesystem::remove_all(directory)); }

  boost::filesystem::path directory;
};

struct MapTestWithParam : public testing::TestWithParam<std::uint32_t> {
  void SetUp() override {
    directory = "/tmp/multimap-MapTestFixture";
    boost::filesystem::remove_all(directory);
    assert(boost::filesystem::create_directory(directory));
  }

  void TearDown() override { assert(boost::filesystem::remove_all(directory)); }

  boost::filesystem::path directory;
};

const auto kTruePredicate = [](const Bytes&) { return true; };
const auto kFalsePredicate = [](const Bytes&) { return false; };
const auto kEmptyProcedure = [](const Bytes&) {};
const auto kEmptyFunction = [](const Bytes&) { return std::string(); };

TEST(MapTest, IsDefaultConstructible) {
  ASSERT_THAT(std::is_default_constructible<Map>::value, Eq(true));
}

TEST(MapTest, DefaultConstructedHasProperState) {
  const Bytes key = "key";
  const Bytes value = "value";
  ASSERT_FALSE(Map().Contains(key));
  ASSERT_EQ(Map().Delete(key), 0);
  ASSERT_EQ(Map().DeleteAll(key, kTruePredicate), 0);
  ASSERT_EQ(Map().DeleteAll(key, kFalsePredicate), 0);
  ASSERT_EQ(Map().DeleteAllEqual(key, value), 0);
  ASSERT_FALSE(Map().DeleteFirst(key, kTruePredicate));
  ASSERT_FALSE(Map().DeleteFirst(key, kFalsePredicate));
  ASSERT_FALSE(Map().DeleteFirstEqual(key, value));
  Map().ForEachKey(kEmptyProcedure);
  ASSERT_EQ(Map().Get(key).NumValues(), 0);
  ASSERT_EQ(Map().GetMutable(key).NumValues(), 0);
  ASSERT_THROW(Map().Put(key, value), std::bad_function_call);
  ASSERT_EQ(Map().ReplaceAll(key, kEmptyFunction), 0);
  ASSERT_EQ(Map().ReplaceAllEqual(key, value, value), 0);
  ASSERT_FALSE(Map().ReplaceFirst(key, kEmptyFunction));
  ASSERT_FALSE(Map().ReplaceFirstEqual(key, value, value));
}

TEST(MapTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_THAT(std::is_copy_constructible<Map>::value, Eq(false));
  ASSERT_THAT(std::is_copy_assignable<Map>::value, Eq(false));
}

TEST(MapTest, IsNotMoveConstructibleOrAssignable) {
  ASSERT_THAT(std::is_move_constructible<Map>::value, Eq(false));
  ASSERT_THAT(std::is_move_assignable<Map>::value, Eq(false));
}

TEST_F(MapTestFixture, OpenThrowsIfFilesAreMissing) {
  Map map;
  Options options;
  options.create_if_missing = false;
  ASSERT_THROW(map.Open(directory, options), std::runtime_error);
}

TEST_F(MapTestFixture, OpenDoesNotThrowIfCreateIsMissingIsTrue) {
  {
    Map map;
    Options options;
    options.create_if_missing = true;
    map.Open(directory, options);
  }
  ASSERT_TRUE(boost::filesystem::exists(directory / Map::name_of_data_file()));
  ASSERT_TRUE(boost::filesystem::exists(directory / Map::name_of_table_file()));
}

TEST_F(MapTestFixture, OpenThrowsIfBlockPoolMemoryIsTooSmall) {
  Map map;
  Options options;
  options.create_if_missing = true;
  options.block_pool_memory = 100;
  options.block_size = 128;
  ASSERT_THROW(map.Open(directory, options), std::runtime_error);
}

TEST_F(MapTestFixture, OpenThrowsIfBlockSizeIsNotPowerOfTwo) {
  Map map;
  Options options;
  options.create_if_missing = true;
  options.block_size = 100;
  ASSERT_THROW(map.Open(directory, options), std::runtime_error);
}

TEST_F(MapTestFixture, OpenThrowsIfCalledTwice) {
  Map map;
  Options options;
  options.create_if_missing = true;
  ASSERT_NO_THROW(map.Open(directory, options));
  ASSERT_THROW(map.Open(directory, options), std::runtime_error);
}

TEST_F(MapTestFixture, PutWithMaxKeySizeWorks) {
  Map map;
  Options options;
  options.create_if_missing = true;
  map.Open(directory, options);
  std::string key(map.max_key_size(), 'k');
  ASSERT_NO_THROW(map.Put(key, "value"));
}

TEST_F(MapTestFixture, PutWithTooLargeKeyThrows) {
  Map map;
  Options options;
  options.create_if_missing = true;
  map.Open(directory, options);
  std::string key(map.max_key_size() + 1, 'k');
  ASSERT_THROW(map.Put(key, "value"), std::runtime_error);
}

TEST_F(MapTestFixture, PutWithMaxValueSizeWorks) {
  Map map;
  Options options;
  options.create_if_missing = true;
  map.Open(directory, options);
  std::string value(map.max_value_size(), 'v');
  ASSERT_NO_THROW(map.Put("key", value));
}

TEST_F(MapTestFixture, PutWithTooLargeValueThrows) {
  Map map;
  Options options;
  options.create_if_missing = true;
  map.Open(directory, options);
  std::string value(map.max_value_size() + 1, 'v');
  ASSERT_THROW(map.Put("key", value), std::runtime_error);
}

TEST_P(MapTestWithParam, PutThenGetWorks) {
  multimap::Options options;
  options.create_if_missing = true;
  multimap::Map map(directory, options);
  for (std::uint32_t k = 0; k != GetParam(); ++k) {
    for (std::uint32_t v = 0; v != GetParam(); ++v) {
      map.Put(std::to_string(k), std::to_string(k + v * v));
    }
  }
  for (std::uint32_t k = 0; k != GetParam(); ++k) {
    auto iter = map.Get(std::to_string(k));
    ASSERT_THAT(iter.NumValues(), Eq(GetParam()));
    iter.SeekToFirst();
    for (std::uint32_t v = 0; iter.HasValue(); iter.Next(), ++v) {
      ASSERT_THAT(iter.GetValue().ToString(), Eq(std::to_string(k + v * v)));
    }
  }
}

TEST_P(MapTestWithParam, PutThenCloseThenOpenThenGetWorks) {
  {
    multimap::Options options;
    options.create_if_missing = true;
    multimap::Map map(directory, options);
    for (std::uint32_t k = 0; k != GetParam(); ++k) {
      for (std::uint32_t v = 0; v != GetParam(); ++v) {
        map.Put(std::to_string(k), std::to_string(k + v * v));
      }
    }
  }
  {
    multimap::Map map(directory, Options());
    for (std::uint32_t k = 0; k != GetParam(); ++k) {
      auto iter = map.Get(std::to_string(k));
      ASSERT_THAT(iter.NumValues(), Eq(GetParam()));
      iter.SeekToFirst();
      for (std::uint32_t v = 0; iter.HasValue(); iter.Next(), ++v) {
        ASSERT_THAT(iter.GetValue().ToString(), Eq(std::to_string(k + v * v)));
      }
    }
  }
}

INSTANTIATE_TEST_CASE_P(Parameterized, MapTestWithParam,
                        testing::Values(0, 1, 2, 10, 100, 1000));

// INSTANTIATE_TEST_CASE_P(ParameterizedLongRunning, MapTestWithParam,
//                        testing::Values(10000, 100000, 1000000));

TEST_F(MapTestFixture, ContainsReturnsFalseForNonExistingKey) {
  multimap::Options options;
  options.create_if_missing = true;
  multimap::Map map(directory, options);
  ASSERT_FALSE(map.Contains("key"));
}

TEST_F(MapTestFixture, ContainsReturnsTrueForExistingKey) {
  multimap::Options options;
  options.create_if_missing = true;
  multimap::Map map(directory, options);
  map.Put("key", "value");
  ASSERT_TRUE(map.Contains("key"));
}

TEST_F(MapTestFixture, ContainsReturnsFalseForDeletedKey) {
  multimap::Options options;
  options.create_if_missing = true;
  multimap::Map map(directory, options);
  map.Put("key", "value");
  map.Delete("key");
  ASSERT_FALSE(map.Contains("key"));
}

TEST_F(MapTestFixture, DeleteReturnsZeroForNonExistingKey) {
  multimap::Options options;
  options.create_if_missing = true;
  multimap::Map map(directory, options);
  ASSERT_EQ(map.Delete("key"), 0);
}

TEST_F(MapTestFixture, DeleteReturnsNumValuesForExistingKey) {
  multimap::Options options;
  options.create_if_missing = true;
  multimap::Map map(directory, options);
  map.Put("key", "1");
  map.Put("key", "2");
  map.Put("key", "3");
  ASSERT_EQ(map.Delete("key"), 3);
}

TEST_F(MapTestFixture, DeleteFirstDeletesOneMatch) {
  multimap::Options options;
  options.create_if_missing = true;
  multimap::Map map(directory, options);

  std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.Put("key", std::to_string(i));
    map.Put("key", std::to_string(i));
  }

  const auto is_500 = [](const multimap::Bytes& value) {
    return value == std::to_string(500);
  };
  ASSERT_FALSE(map.DeleteFirst("key", is_500));
  ASSERT_EQ(map.Get("key").NumValues(), num_values);

  const auto is_250 = [](const multimap::Bytes& value) {
    return value == std::to_string(250);
  };
  ASSERT_TRUE(map.DeleteFirst("key", is_250));
  ASSERT_EQ(map.Get("key").NumValues(), num_values - 1);
}

TEST_F(MapTestFixture, DeleteFirstEqualDeletesOneMatch) {
  multimap::Options options;
  options.create_if_missing = true;
  multimap::Map map(directory, options);

  std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.Put("key", std::to_string(i));
    map.Put("key", std::to_string(i));
  }

  ASSERT_FALSE(map.DeleteFirstEqual("key", "500"));
  ASSERT_EQ(map.Get("key").NumValues(), num_values);

  ASSERT_TRUE(map.DeleteFirstEqual("key", "250"));
  ASSERT_EQ(map.Get("key").NumValues(), num_values - 1);
}

TEST_F(MapTestFixture, DeleteAllDeletesAllMatches) {
  multimap::Options options;
  options.create_if_missing = true;
  multimap::Map map(directory, options);

  std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.Put("key", std::to_string(i));
    map.Put("key", std::to_string(i));
  }

  const auto is_500 = [](const multimap::Bytes& value) {
    return value == std::to_string(500);
  };
  ASSERT_EQ(map.DeleteAll("key", is_500), 0);
  ASSERT_EQ(map.Get("key").NumValues(), num_values);

  const auto is_250 = [](const multimap::Bytes& value) {
    return value == std::to_string(250);
  };
  ASSERT_EQ(map.DeleteAll("key", is_250), 2);
  ASSERT_EQ(map.Get("key").NumValues(), num_values - 2);
}

TEST_F(MapTestFixture, DeleteAllEqualDeletesAllMatches) {
  multimap::Options options;
  options.create_if_missing = true;
  multimap::Map map(directory, options);

  std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.Put("key", std::to_string(i));
    map.Put("key", std::to_string(i));
  }

  ASSERT_EQ(map.DeleteAllEqual("key", "500"), 0);
  ASSERT_EQ(map.Get("key").NumValues(), num_values);

  ASSERT_EQ(map.DeleteAllEqual("key", "250"), 2);
  ASSERT_EQ(map.Get("key").NumValues(), num_values - 2);
}

TEST_F(MapTestFixture, ReplaceFirstReplacesOneMatch) {
  multimap::Options options;
  options.create_if_missing = true;
  multimap::Map map(directory, options);

  std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.Put("key", std::to_string(i));
    map.Put("key", std::to_string(i));
  }

  const auto no_match = [](const multimap::Bytes& /* value */) { return ""; };
  ASSERT_FALSE(map.ReplaceFirst("key", no_match));
  ASSERT_EQ(map.Get("key").NumValues(), num_values);

  const auto map_250_to_2500 = [](const multimap::Bytes& value) {
    return (value.ToString() == "250") ? "2500" : "";
  };
  ASSERT_TRUE(map.ReplaceFirst("key", map_250_to_2500));
  ASSERT_EQ(map.Get("key").NumValues(), num_values);
  // TODO Iterate list and check if replacement.
}

TEST_F(MapTestFixture, ReplaceFirstEqualReplacesOneMatch) {
  multimap::Options options;
  options.create_if_missing = true;
  multimap::Map map(directory, options);

  std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.Put("key", std::to_string(i));
    map.Put("key", std::to_string(i));
  }

  ASSERT_FALSE(map.ReplaceFirstEqual("key", "500", "2500"));
  ASSERT_EQ(map.Get("key").NumValues(), num_values);

  ASSERT_TRUE(map.ReplaceFirstEqual("key", "250", "2500"));
  ASSERT_EQ(map.Get("key").NumValues(), num_values);
  // TODO Iterate list and check replacement.
}

TEST_F(MapTestFixture, ReplaceAllReplacesAllMatches) {
  multimap::Options options;
  options.create_if_missing = true;
  multimap::Map map(directory, options);

  std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.Put("key", std::to_string(i));
    map.Put("key", std::to_string(i));
  }

  const auto no_match = [](const multimap::Bytes& /* value */) { return ""; };
  ASSERT_EQ(map.ReplaceAll("key", no_match), 0);
  ASSERT_EQ(map.Get("key").NumValues(), num_values);

  const auto map_250_to_2500 = [](const multimap::Bytes& value) {
    return (value.ToString() == "250") ? "2500" : "";
  };
  ASSERT_EQ(map.ReplaceAll("key", map_250_to_2500), 2);
  ASSERT_EQ(map.Get("key").NumValues(), num_values);
  // TODO Iterate list and check replacement.
}

TEST_F(MapTestFixture, ReplaceAllEqualReplacesAllMatches) {
  multimap::Options options;
  options.create_if_missing = true;
  multimap::Map map(directory, options);

  std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.Put("key", std::to_string(i));
    map.Put("key", std::to_string(i));
  }

  ASSERT_EQ(map.ReplaceAllEqual("key", "500", "2500"), 0);
  ASSERT_EQ(map.Get("key").NumValues(), num_values);

  ASSERT_EQ(map.ReplaceAllEqual("key", "250", "2500"), 2);
  ASSERT_EQ(map.Get("key").NumValues(), num_values);
  // TODO Iterate list and check replacement.
}

}  // namespace multimap
