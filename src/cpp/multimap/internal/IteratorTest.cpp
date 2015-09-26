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
#include "multimap/internal/System.hpp"
#include "multimap/Map.hpp"

namespace multimap {
namespace internal {

using testing::Eq;
using testing::Ne;

typedef Iterator<false> Iter;
typedef Iterator<true> ConstIter;

struct IteratorTestFixture : testing::TestWithParam<std::uint32_t> {
  void SetUp() override {
    directory = "/tmp/multimap-IteratorTestFixture";
    boost::filesystem::remove_all(directory);
    assert(boost::filesystem::create_directory(directory));

    Options options;
    options.create_if_missing = true;
    map.reset(new Map(directory, options));

    key = "key";
    for (std::size_t i = 0; i != GetParam(); ++i) {
      map->put(key, std::to_string(i));
    }
  }

  void TearDown() override {
    map.reset();  // We need the d'tor call here.
    assert(boost::filesystem::remove_all(directory));
  }

  std::string key;
  std::unique_ptr<Map> map;
  boost::filesystem::path directory;
};

struct ConstIterTest : public IteratorTestFixture {
  ConstIter getIterator() { return map->get(key); }
};

struct IterTest : public IteratorTestFixture {
  Iter getIterator() { return map->getMutable(key); }
};

TEST(IterTest, IsDefaultConstructible) {
  ASSERT_THAT(std::is_default_constructible<Iter>::value, Eq(true));
}

TEST(IterTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_THAT(std::is_copy_constructible<Iter>::value, Eq(false));
  ASSERT_THAT(std::is_copy_assignable<Iter>::value, Eq(false));
}

TEST(IterTest, IsMoveConstructibleAndAssignable) {
  ASSERT_THAT(std::is_move_constructible<Iter>::value, Eq(true));
  ASSERT_THAT(std::is_move_assignable<Iter>::value, Eq(true));
}

TEST(IterTest, DefaultConstructedHasProperState) {
  ASSERT_THAT(Iter().available(), Eq(0));
  ASSERT_THAT(Iter().hasNext(), Eq(false));
  ASSERT_THAT(Iter().available(), Eq(0));
}

TEST_P(IterTest, IterateAll) {
  Iter iter = getIterator();
  ASSERT_THAT(iter.available(), Eq(GetParam()));
  for (std::size_t i = 0; iter.hasNext(); ++i) {
    ASSERT_THAT(iter.available(), Eq(GetParam() - i));
    ASSERT_THAT(iter.next(), Eq(std::to_string(i)));
  }
  ASSERT_THAT(iter.hasNext(), Eq(false));
  ASSERT_THAT(iter.available(), Eq(0));
}

TEST_P(IterTest, IterateOnceAndRemoveEvery23thValue) {
  Iter iter = getIterator();
  std::size_t num_removed = 0;
  for (std::size_t i = 0; iter.hasNext(); ++i) {
    ASSERT_THAT(iter.next(), Eq(std::to_string(i)));
    if (i % 23 == 0) {
      iter.remove();
      ++num_removed;
    }
  }
  ASSERT_THAT(iter.hasNext(), Eq(false));
  ASSERT_THAT(iter.available(), Eq(0));
}

TEST_P(IterTest, IterateTwiceAndRemoveEvery23thValueIn1stRun) {
  std::size_t num_removed = 0;
  {
    Iter iter = getIterator();
    for (std::size_t i = 0; iter.hasNext(); ++i) {
      iter.next();
      if (i % 23 == 0) {
        iter.remove();
        ++num_removed;
      }
    }
    ASSERT_THAT(iter.hasNext(), Eq(false));
    ASSERT_THAT(iter.available(), Eq(0));
  }
  {
    Iter iter = getIterator();
    ASSERT_THAT(iter.available(), Eq(GetParam() - num_removed));
    for (std::size_t i = 0; iter.hasNext(); ++i) {
      if (i % 23 != 0) {
        ASSERT_THAT(std::stoi(iter.next().toString()) % 23, Ne(0));
      }
    }
    ASSERT_THAT(iter.hasNext(), Eq(false));
    ASSERT_THAT(iter.available(), Eq(0));
  }
}

TEST(ConstIterTest, IsDefaultConstructible) {
  ASSERT_THAT(std::is_default_constructible<ConstIter>::value, Eq(true));
}

TEST(ConstIterTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_THAT(std::is_copy_constructible<ConstIter>::value, Eq(false));
  ASSERT_THAT(std::is_copy_assignable<ConstIter>::value, Eq(false));
}

TEST(ConstIterTest, IsMoveConstructibleAndAssignable) {
  ASSERT_THAT(std::is_move_constructible<ConstIter>::value, Eq(true));
  ASSERT_THAT(std::is_move_assignable<ConstIter>::value, Eq(true));
}

TEST(ConstIterTest, DefaultConstructedHasProperState) {
  ASSERT_THAT(Iter().hasNext(), Eq(false));
  ASSERT_THAT(Iter().available(), Eq(0));
}

TEST_P(ConstIterTest, IterateAll) {
  ConstIter iter = getIterator();
  ASSERT_THAT(iter.available(), Eq(GetParam()));
  for (std::size_t i = 0; iter.hasNext(); ++i) {
    ASSERT_THAT(iter.available(), Eq(GetParam() - i));
    ASSERT_THAT(iter.next(), Eq(std::to_string(i)));
  }
  ASSERT_THAT(iter.hasNext(), Eq(false));
  ASSERT_THAT(iter.available(), Eq(0));
}

INSTANTIATE_TEST_CASE_P(Parameterized, IterTest,
                        testing::Values(0, 1, 2, 10, 100, 1000, 10000));

INSTANTIATE_TEST_CASE_P(Parameterized, ConstIterTest,
                        testing::Values(0, 1, 2, 10, 100, 1000, 10000));

// TODO Micro-benchmark this.
// INSTANTIATE_TEST_CASE_P(ParameterizedLongRunning, IterTest,
//                        testing::Values(100000, 1000000));

// TODO Micro-benchmark this.
// INSTANTIATE_TEST_CASE_P(ParameterizedLongRunning, ConstIterTest,
//                        testing::Values(100000, 1000000));

}  // namespace internal
}  // namespace multimap
