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

const Callables::Predicate TRUE_PREDICATE = [](const Bytes&) { return true; };
const Callables::Predicate FALSE_PREDICATE = [](const Bytes&) { return false; };
const Callables::Procedure NULL_PROCEDURE = [](const Bytes&) {};
const Callables::Function NULL_FUNCTION = [](const Bytes&) { return ""; };
const Callables::Procedure2<Map::ListIterator> NULL_PROCEDURE2 =
    [](const Bytes&, Map::ListIterator&&) {};

TEST(MapTest, IsDefaultConstructible) {
  ASSERT_THAT(std::is_default_constructible<Map>::value, Eq(true));
}

TEST(MapTest, DefaultConstructedHasProperState) {
  const Bytes key = "key";
  const Bytes value = "value";
  ASSERT_THROW(Map().put(key, value), mt::AssertionError);
  ASSERT_THROW(Map().get(key).available(), mt::AssertionError);
  ASSERT_THROW(Map().getMutable(key).available(), mt::AssertionError);
  ASSERT_THROW(Map().contains(key), mt::AssertionError);
  ASSERT_THROW(Map().remove(key), mt::AssertionError);
  ASSERT_THROW(Map().removeAll(key, TRUE_PREDICATE), mt::AssertionError);
  ASSERT_THROW(Map().removeAllEqual(key, value), mt::AssertionError);
  ASSERT_THROW(Map().removeFirst(key, TRUE_PREDICATE), mt::AssertionError);
  ASSERT_THROW(Map().removeFirstEqual(key, value), mt::AssertionError);
  ASSERT_THROW(Map().replaceAll(key, NULL_FUNCTION), mt::AssertionError);
  ASSERT_THROW(Map().replaceAllEqual(key, value, value), mt::AssertionError);
  ASSERT_THROW(Map().replaceFirst(key, NULL_FUNCTION), mt::AssertionError);
  ASSERT_THROW(Map().replaceFirstEqual(key, value, value), mt::AssertionError);
  ASSERT_THROW(Map().forEachKey(NULL_PROCEDURE), mt::AssertionError);
  ASSERT_THROW(Map().forEachValue(key, NULL_PROCEDURE), mt::AssertionError);
  ASSERT_THROW(Map().forEachValue(key, TRUE_PREDICATE), mt::AssertionError);
  ASSERT_THROW(Map().forEachEntry(NULL_PROCEDURE2), mt::AssertionError);
  ASSERT_THROW(Map().getProperties(), mt::AssertionError);
}

TEST(MapTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_THAT(std::is_copy_constructible<Map>::value, Eq(false));
  ASSERT_THAT(std::is_copy_assignable<Map>::value, Eq(false));
}

TEST(MapTest, IsMoveConstructibleOrAssignable) {
  ASSERT_THAT(std::is_move_constructible<Map>::value, Eq(true));
  ASSERT_THAT(std::is_move_assignable<Map>::value, Eq(true));
}

TEST_F(MapTestFixture, OpenThrowsIfFilesAreMissing) {
  Options options;
  options.create_if_missing = false;
  ASSERT_THROW(Map(directory, options), std::runtime_error);
}

TEST_F(MapTestFixture, OpenDoesNotThrowIfCreateIsMissingIsTrue) {
  {
    Options options;
    options.create_if_missing = true;
    Map(directory, options);
  }
  ASSERT_FALSE(boost::filesystem::is_empty(directory));
}

TEST_F(MapTestFixture, OpenThrowsIfMapExistsAndErrorIfExistsIsTrue) {
  {
    Options options;
    options.create_if_missing = true;
    Map(directory, options);
  }
  Options options;
  options.error_if_exists = true;
  ASSERT_THROW(Map(directory, options), std::runtime_error);
}

TEST_F(MapTestFixture, OpenThrowsIfBlockSizeIsNotPowerOfTwo) {
  Options options;
  options.create_if_missing = true;
  options.block_size = 100;
  ASSERT_THROW(Map(directory, options), std::runtime_error);
}

TEST_F(MapTestFixture, PutWithMaxKeySizeWorks) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);
  std::string key(map.max_key_size(), 'k');
  ASSERT_NO_THROW(map.put(key, "value"));
}

TEST_F(MapTestFixture, PutWithTooLargeKeyThrows) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);
  std::string key(map.max_key_size() + 1, 'k');
  ASSERT_THROW(map.put(key, "value"), std::runtime_error);
}

TEST_F(MapTestFixture, PutWithMaxValueSizeWorks) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);
  std::string value(map.max_value_size(), 'v');
  ASSERT_NO_THROW(map.put("key", value));
}

TEST_F(MapTestFixture, PutWithTooLargeValueThrows) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);
  std::string value(map.max_value_size() + 1, 'v');
  ASSERT_THROW(map.put("key", value), std::runtime_error);
}

TEST_P(MapTestWithParam, PutThenGetWorks) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);
  for (std::uint32_t k = 0; k != GetParam(); ++k) {
    for (std::uint32_t v = 0; v != GetParam(); ++v) {
      map.put(std::to_string(k), std::to_string(k + v * v));
    }
  }
  for (std::uint32_t k = 0; k != GetParam(); ++k) {
    auto iter = map.get(std::to_string(k));
    ASSERT_THAT(iter.available(), Eq(GetParam()));
    for (std::uint32_t v = 0; iter.hasNext(); ++v) {
      ASSERT_THAT(iter.next().toString(), Eq(std::to_string(k + v * v)));
    }
    ASSERT_THAT(iter.available(), Eq(0));
  }
}

TEST_P(MapTestWithParam, PutThenCloseThenOpenThenGetWorks) {
  {
    Options options;
    options.create_if_missing = true;
    Map map(directory, options);
    for (std::uint32_t k = 0; k != GetParam(); ++k) {
      for (std::uint32_t v = 0; v != GetParam(); ++v) {
        map.put(std::to_string(k), std::to_string(k + v * v));
      }
    }
  }
  {
    Map map(directory, Options());
    for (std::uint32_t k = 0; k != GetParam(); ++k) {
      auto iter = map.get(std::to_string(k));
      ASSERT_THAT(iter.available(), Eq(GetParam()));
      for (std::uint32_t v = 0; iter.hasNext(); ++v) {
        ASSERT_THAT(iter.next().toString(), Eq(std::to_string(k + v * v)));
      }
      ASSERT_THAT(iter.available(), Eq(0));
    }
  }
}

INSTANTIATE_TEST_CASE_P(Parameterized, MapTestWithParam,
                        testing::Values(0, 1, 2, 10, 100, 1000));

// INSTANTIATE_TEST_CASE_P(ParameterizedLongRunning, MapTestWithParam,
//                        testing::Values(10000, 100000, 1000000));

TEST_F(MapTestFixture, ContainsReturnsFalseForNonExistingKey) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);
  ASSERT_FALSE(map.contains("key"));
}

TEST_F(MapTestFixture, ContainsReturnsTrueForExistingKey) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);
  map.put("key", "value");
  ASSERT_TRUE(map.contains("key"));
}

TEST_F(MapTestFixture, ContainsReturnsFalseForDeletedKey) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);
  map.put("key", "value");
  map.remove("key");
  ASSERT_FALSE(map.contains("key"));
}

TEST_F(MapTestFixture, DeleteReturnsZeroForNonExistingKey) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);
  ASSERT_EQ(map.remove("key"), 0);
}

TEST_F(MapTestFixture, DeleteReturnsNumValuesForExistingKey) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);
  map.put("key", "1");
  map.put("key", "2");
  map.put("key", "3");
  ASSERT_EQ(map.remove("key"), 3);
}

TEST_F(MapTestFixture, DeleteFirstDeletesOneMatch) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);

  const std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.put("key", std::to_string(i));
    map.put("key", std::to_string(i));
  }

  const auto is_500 =
      [](const Bytes& value) { return value == std::to_string(500); };
  ASSERT_FALSE(map.removeFirst("key", is_500));
  ASSERT_EQ(map.get("key").available(), num_values);

  const auto is_250 =
      [](const Bytes& value) { return value == std::to_string(250); };
  ASSERT_TRUE(map.removeFirst("key", is_250));
  ASSERT_EQ(map.get("key").available(), num_values - 1);
}

TEST_F(MapTestFixture, DeleteFirstEqualDeletesOneMatch) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);

  const std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.put("key", std::to_string(i));
    map.put("key", std::to_string(i));
  }

  ASSERT_FALSE(map.removeFirstEqual("key", "500"));
  ASSERT_EQ(map.get("key").available(), num_values);

  ASSERT_TRUE(map.removeFirstEqual("key", "250"));
  ASSERT_EQ(map.get("key").available(), num_values - 1);
}

TEST_F(MapTestFixture, DeleteAllDeletesAllMatches) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);

  const std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.put("key", std::to_string(i));
    map.put("key", std::to_string(i));
  }

  const auto is_500 =
      [](const Bytes& value) { return value == std::to_string(500); };
  ASSERT_EQ(map.removeAll("key", is_500), 0);
  ASSERT_EQ(map.get("key").available(), num_values);

  const auto is_250 =
      [](const Bytes& value) { return value == std::to_string(250); };
  ASSERT_EQ(map.removeAll("key", is_250), 2);
  ASSERT_EQ(map.get("key").available(), num_values - 2);
}

TEST_F(MapTestFixture, DeleteAllEqualDeletesAllMatches) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);

  const std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.put("key", std::to_string(i));
    map.put("key", std::to_string(i));
  }

  ASSERT_EQ(map.removeAllEqual("key", "500"), 0);
  ASSERT_EQ(map.get("key").available(), num_values);

  ASSERT_EQ(map.removeAllEqual("key", "250"), 2);
  ASSERT_EQ(map.get("key").available(), num_values - 2);
}

TEST_F(MapTestFixture, ReplaceFirstReplacesOneMatch) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);

  const std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.put("key", std::to_string(i));
    map.put("key", std::to_string(i));
  }

  ASSERT_FALSE(map.replaceFirst("key", NULL_FUNCTION));
  ASSERT_EQ(map.get("key").available(), num_values);

  const Callables::Function map_250_to_2500 = [](const Bytes& value) {
    return (value.toString() == "250") ? "2500" : "";
  };
  ASSERT_TRUE(map.replaceFirst("key", map_250_to_2500));
  ASSERT_EQ(map.get("key").available(), num_values);
  // TODO Iterate list and check replacement.
}

TEST_F(MapTestFixture, ReplaceFirstEqualReplacesOneMatch) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);

  const std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.put("key", std::to_string(i));
    map.put("key", std::to_string(i));
  }

  ASSERT_FALSE(map.replaceFirstEqual("key", "500", "2500"));
  ASSERT_EQ(map.get("key").available(), num_values);

  ASSERT_TRUE(map.replaceFirstEqual("key", "250", "2500"));
  ASSERT_EQ(map.get("key").available(), num_values);
  // TODO Iterate list and check replacement.
}

TEST_F(MapTestFixture, ReplaceAllReplacesAllMatches) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);

  const std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.put("key", std::to_string(i));
    map.put("key", std::to_string(i));
  }

  ASSERT_EQ(map.replaceAll("key", NULL_FUNCTION), 0);
  ASSERT_EQ(map.get("key").available(), num_values);

  const Callables::Function map_250_to_2500 = [](const Bytes& value) {
    return (value.toString() == "250") ? "2500" : "";
  };
  ASSERT_EQ(map.replaceAll("key", map_250_to_2500), 2);
  ASSERT_EQ(map.get("key").available(), num_values);
  // TODO Iterate list and check replacement.
}

TEST_F(MapTestFixture, ReplaceAllEqualReplacesAllMatches) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);

  const std::size_t num_values = 1000;
  for (std::size_t i = 0; i != num_values / 2; ++i) {
    map.put("key", std::to_string(i));
    map.put("key", std::to_string(i));
  }

  ASSERT_EQ(map.replaceAllEqual("key", "500", "2500"), 0);
  ASSERT_EQ(map.get("key").available(), num_values);

  ASSERT_EQ(map.replaceAllEqual("key", "250", "2500"), 2);
  ASSERT_EQ(map.get("key").available(), num_values);
  // TODO Iterate list and check replacement.
}

TEST_F(MapTestFixture, IteratorWritesBackMutatedBlocks) {
  Options options;
  options.create_if_missing = true;
  Map map(directory, options);

  const std::size_t num_values = 100000;
  for (std::size_t i = 0; i != num_values; ++i) {
    map.put("key", "value" + std::to_string(i));
  }

  {
    auto iter = map.getMutable("key");
    ASSERT_EQ(iter.available(), num_values);
    for (std::size_t i = 0; i != num_values / 2; ++i) {
      if (i == 113) {
        ASSERT_TRUE(iter.hasNext());
        ASSERT_EQ(iter.next().toString(), "value" + std::to_string(i));
      } else {
        ASSERT_TRUE(iter.hasNext());
        ASSERT_EQ(iter.next().toString(), "value" + std::to_string(i));
      }
      iter.remove();
    }
  }

  auto iter = map.get("key");
  ASSERT_EQ(iter.available(), num_values / 2);
  for (std::size_t i = num_values / 2; i != num_values; ++i) {
    ASSERT_TRUE(iter.hasNext());
    ASSERT_EQ(iter.next().toString(), "value" + std::to_string(i));
  }
}

}  // namespace multimap
