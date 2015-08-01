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

#include <boost/filesystem/operations.hpp>
#include "gmock/gmock.h"
#include "multimap/Map.hpp"

namespace multimap {

using testing::Eq;

struct MapTestWithParam : testing::TestWithParam<std::uint32_t> {
  void SetUp() override {
    directory = "/tmp/multimap-MapTest";
    boost::filesystem::remove_all(directory);
    assert(boost::filesystem::create_directory(directory));
  }

  void TearDown() override { assert(boost::filesystem::remove_all(directory)); }

  boost::filesystem::path directory;
};

TEST_P(MapTestWithParam, PutThenGetWorks) {
  multimap::Map map(directory, Options());
  for (std::uint32_t k = 0; k != GetParam(); ++k) {
    for (std::uint32_t v = 0; v != GetParam(); ++v) {
      map.Put(std::to_string(k), std::to_string(k + v * v));
    }
  }
  for (std::uint32_t k = 0; k != GetParam(); ++k) {
    auto iter = map.Get(std::to_string(k));
    ASSERT_THAT(iter.Size(), Eq(GetParam()));
    iter.SeekToFirst();
    for (std::uint32_t v = 0; iter.Valid(); iter.Next(), ++v) {
      ASSERT_THAT(iter.Value().ToString(), Eq(std::to_string(k + v * v)));
    }
  }
}

TEST_P(MapTestWithParam, PutThenCloseThenOpenThenGetWorks) {
  {
    multimap::Map map(directory, Options());
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
      ASSERT_THAT(iter.Size(), Eq(GetParam()));
      iter.SeekToFirst();
      for (std::uint32_t v = 0; iter.Valid(); iter.Next(), ++v) {
        ASSERT_THAT(iter.Value().ToString(), Eq(std::to_string(k + v * v)));
      }
    }
  }
}

INSTANTIATE_TEST_CASE_P(Parameterized, MapTestWithParam,
                        testing::Values(0, 1, 2, 10, 100, 1000));

// INSTANTIATE_TEST_CASE_P(Parameterized, MapTestWithParam,
//                        testing::Values(10000, 100000, 1000000));

}  // namespace multimap
