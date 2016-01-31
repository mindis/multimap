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
#include <boost/filesystem/operations.hpp>
#include "gmock/gmock.h"
#include "multimap/callables.hpp"
#include "multimap/Map.hpp"

namespace multimap {

using testing::Eq;

const auto NULL_PROCEDURE = [](const Bytes&) {};
const auto TRUE_PREDICATE = [](const Bytes&) { return true; };
const auto FALSE_PREDICATE = [](const Bytes&) { return false; };
const auto EMPTY_FUNCTION = [](const Bytes&) { return std::string(); };
const auto NULL_BINARY_PROCEDURE = [](const Bytes&, Map::Iterator&&) {};
const auto IS_ODD = [](const Bytes& b) { return std::stoul(b.toString()) % 2; };

std::unique_ptr<Map> openOrCreateMap(const boost::filesystem::path& directory) {
  Options options;
  options.create_if_missing = true;
  return std::unique_ptr<Map>(new Map(directory, options));
}

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
    boost::filesystem::remove_all(directory);
    boost::filesystem::create_directory(directory);
  }

  void TearDown() override { boost::filesystem::remove_all(directory); }

  const boost::filesystem::path directory = "/tmp/multimap.MapTestFixture";
};

TEST_F(MapTestFixture, ConstructorThrowsIfNotExist) {
  ASSERT_THROW(Map(this->directory), std::runtime_error);
  // GCC complains when not using 'this' pointer.
}

TEST_F(MapTestFixture, ConstructorWithDefaultOptionsThrowsIfNotExist) {
  Options options;
  ASSERT_THROW(Map(directory, options), std::runtime_error);
}

TEST_F(MapTestFixture, ConstructorWithCreateIfMissingDoesNotThrow) {
  Options options;
  options.create_if_missing = true;
  ASSERT_NO_THROW(Map(directory, options));
}

TEST_F(MapTestFixture, ConstructorWithErrorIfExistsThrowsIfExists) {
  openOrCreateMap(directory);
  Options options;
  options.error_if_exists = true;
  options.create_if_missing = true;
  ASSERT_THROW(Map(directory, options), std::runtime_error);
}

TEST_F(MapTestFixture, ConstructorThrowsIfBlockSizeIsNotPowerOfTwo) {
  Options options;
  options.block_size = 100;
  options.create_if_missing = true;
  ASSERT_THROW(Map(directory, options), std::runtime_error);
}

struct MapTestWithParam : public testing::TestWithParam<int> {
  void SetUp() override {
    boost::filesystem::remove_all(directory);
    boost::filesystem::create_directory(directory);
  }

  void TearDown() override { boost::filesystem::remove_all(directory); }

  const boost::filesystem::path directory = "/tmp/multimap.MapTestWithParam";
};

TEST_P(MapTestWithParam, PutDataThenReadAll) {
  auto map = openOrCreateMap(directory);
  for (auto k = 0; k != GetParam(); ++k) {
    for (auto v = 0; v != GetParam(); ++v) {
      map->put(std::to_string(k), std::to_string(v));
    }
  }
  for (auto k = 0; k != GetParam(); ++k) {
    auto iter = map->get(std::to_string(k));
    ASSERT_THAT(iter.available(), Eq(GetParam()));
    for (auto v = 0; iter.hasNext(); ++v) {
      ASSERT_THAT(iter.next(), Eq(std::to_string(v)));
    }
    ASSERT_THAT(iter.available(), Eq(0));
    ASSERT_FALSE(iter.hasNext());
  }
}

TEST_P(MapTestWithParam, PutDataThenReopenThenReadAll) {
  {
    auto map = openOrCreateMap(directory);
    for (auto k = 0; k != GetParam(); ++k) {
      for (auto v = 0; v != GetParam(); ++v) {
        map->put(std::to_string(k), std::to_string(v));
      }
    }
  }
  {
    auto map = openOrCreateMap(directory);
    for (auto k = 0; k != GetParam(); ++k) {
      auto iter = map->get(std::to_string(k));
      ASSERT_THAT(iter.available(), Eq(GetParam()));
      for (auto v = 0; iter.hasNext(); ++v) {
        ASSERT_THAT(iter.next(), Eq(std::to_string(v)));
      }
      ASSERT_THAT(iter.available(), Eq(0));
      ASSERT_FALSE(iter.hasNext());
    }
  }
}

TEST_P(MapTestWithParam, GetTotalStatsReturnsCorrectValuesAfterPuttingValues) {
  {
    auto map = openOrCreateMap(directory);
    for (auto k = 0; k != GetParam(); ++k) {
      for (auto v = 0; v != GetParam(); ++v) {
        map->put(std::to_string(k), std::to_string(v));
      }
    }

    const auto stats = map->getTotalStats();
    ASSERT_THAT(stats.list_size_avg, Eq(GetParam()));
    ASSERT_THAT(stats.list_size_max, Eq(GetParam()));
    ASSERT_THAT(stats.list_size_min, Eq(GetParam()));
    ASSERT_THAT(stats.num_keys_total, Eq(GetParam()));
    ASSERT_THAT(stats.num_values_total, Eq(GetParam() * GetParam()));
    ASSERT_THAT(stats.num_values_valid, Eq(GetParam() * GetParam()));
  }

  auto map = openOrCreateMap(directory);
  const auto stats = map->getTotalStats();
  ASSERT_THAT(stats.list_size_avg, Eq(GetParam()));
  ASSERT_THAT(stats.list_size_max, Eq(GetParam()));
  ASSERT_THAT(stats.list_size_min, Eq(GetParam()));
  ASSERT_THAT(stats.num_keys_total, Eq(GetParam()));
  ASSERT_THAT(stats.num_values_total, Eq(GetParam() * GetParam()));
  ASSERT_THAT(stats.num_values_valid, Eq(GetParam() * GetParam()));
}

TEST_P(MapTestWithParam, GetTotalStatsReturnsCorrectValuesAfterRemovingKeys) {
  Map::Stats stats_backup;
  {
    auto map = openOrCreateMap(directory);
    for (auto k = 0; k != GetParam(); ++k) {
      for (auto v = 0; v != GetParam(); ++v) {
        map->put(std::to_string(k), std::to_string(v));
      }
    }

    const auto num_keys_removed =
        map->removeKey(std::to_string(0)) + map->removeKeys(IS_ODD);

    stats_backup = map->getTotalStats();
    ASSERT_THAT(stats_backup.num_keys_total, Eq(GetParam()));
    ASSERT_THAT(stats_backup.num_keys_valid, Eq(GetParam() - num_keys_removed));

    const auto num_values_removed = num_keys_removed * GetParam();
    const auto exp_num_values_total = GetParam() * GetParam();
    const auto exp_num_values_valid = exp_num_values_total - num_values_removed;
    ASSERT_THAT(stats_backup.num_values_total, Eq(exp_num_values_total));
    ASSERT_THAT(stats_backup.num_values_valid, Eq(exp_num_values_valid));
  }

  auto map = openOrCreateMap(directory);
  const auto stats = map->getTotalStats();
  ASSERT_THAT(stats.num_keys_total, Eq(stats_backup.num_keys_valid));
  ASSERT_THAT(stats.num_keys_valid, Eq(stats_backup.num_keys_valid));
  ASSERT_THAT(stats.num_values_total, Eq(stats_backup.num_values_total));
  ASSERT_THAT(stats.num_values_valid, Eq(stats_backup.num_values_valid));
}

TEST_P(MapTestWithParam, GetTotalStatsReturnsCorrectValuesAfterRemovingValues) {
  Map::Stats stats_backup;
  {
    auto map = openOrCreateMap(directory);
    for (auto k = 0; k != GetParam(); ++k) {
      for (auto v = 0; v != GetParam(); ++v) {
        map->put(std::to_string(k), std::to_string(v));
      }
    }

    const auto key = std::to_string(0);
    const auto val = std::to_string(0);
    const auto num_values_removed =
        map->removeValue(key, Equal(val)) + map->removeValues(key, IS_ODD);

    stats_backup = map->getTotalStats();
    const auto exp_num_values_total = GetParam() * GetParam();
    const auto exp_num_values_valid = exp_num_values_total - num_values_removed;
    ASSERT_THAT(stats_backup.num_values_total, Eq(exp_num_values_total));
    ASSERT_THAT(stats_backup.num_values_valid, Eq(exp_num_values_valid));
  }

  auto map = openOrCreateMap(directory);
  const auto stats = map->getTotalStats();
  ASSERT_THAT(stats.num_keys_total, Eq(stats_backup.num_keys_valid));
  ASSERT_THAT(stats.num_keys_valid, Eq(stats_backup.num_keys_valid));
  ASSERT_THAT(stats.num_values_total, Eq(stats_backup.num_values_total));
  ASSERT_THAT(stats.num_values_valid, Eq(stats_backup.num_values_valid));
}

INSTANTIATE_TEST_CASE_P(Parameterized, MapTestWithParam,
                        testing::Values(0, 1, 2, 10, 100, 1000));

// INSTANTIATE_TEST_CASE_P(ParameterizedLongRunning, MapTestWithParam,
//                         testing::Values(10000, 100000));

} // namespace multimap
