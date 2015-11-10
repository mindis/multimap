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
#include <gmock/gmock.h>
#include "multimap/internal/System.hpp"
#include "multimap/Map.hpp"

namespace multimap {
namespace internal {

using testing::Eq;
using testing::Ne;

struct IteratorTestFixture : testing::TestWithParam<std::uint32_t> {
  void SetUp() override {
    directory = "/tmp/multimap.IteratorTestFixture";
    boost::filesystem::remove_all(directory);
    assert(boost::filesystem::create_directory(directory));

    Options options;
    options.create_if_missing = true;
    map = Map(directory, options);

    key = "key";
    for (std::size_t i = 0; i != GetParam(); ++i) {
      map.put(key, std::to_string(i));
    }
  }

  void TearDown() override {
    map = Map(); // We need the d'tor call here.
    assert(boost::filesystem::remove_all(directory));
  }

  Map map;
  std::string key;
  boost::filesystem::path directory;
};

struct SharedIteratorTest : public IteratorTestFixture {
  SharedListIterator getIterator() { return map.get(key); }
};

TEST(SharedIteratorTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<SharedListIterator>::value);
}

TEST(SharedIteratorTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<SharedListIterator>::value);
  ASSERT_FALSE(std::is_copy_assignable<SharedListIterator>::value);
}

TEST(SharedIteratorTest, IsMoveConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_move_constructible<SharedListIterator>::value);
  ASSERT_TRUE(std::is_move_assignable<SharedListIterator>::value);
}

TEST(SharedIteratorTest, DefaultConstructedHasProperState) {
  ASSERT_THAT(UniqueListIterator().available(), Eq(0));
  ASSERT_THAT(UniqueListIterator().hasNext(), Eq(false));
  // Calling `next()` or `peekNext()` or `remove()` is undefined behavior.
}

TEST_P(SharedIteratorTest, Iterate) {
  SharedListIterator iter = getIterator();
  ASSERT_THAT(iter.available(), Eq(GetParam()));
  for (std::size_t i = 0; iter.hasNext(); ++i) {
    ASSERT_THAT(iter.available(), Eq(GetParam() - i));
    ASSERT_THAT(iter.next(), Eq(std::to_string(i)));
  }
  ASSERT_THAT(iter.hasNext(), Eq(false));
  ASSERT_THAT(iter.available(), Eq(0));
}

INSTANTIATE_TEST_CASE_P(Parameterized, SharedIteratorTest,
                        testing::Values(0, 1, 2, 10, 100, 1000, 10000));

struct UniqueListIteratorTest : public IteratorTestFixture {
  UniqueListIterator getIterator() { return map.getMutable(key); }
};

TEST(UniqueListIteratorTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<UniqueListIterator>::value);
}

TEST(UniqueListIteratorTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<UniqueListIterator>::value);
  ASSERT_FALSE(std::is_copy_assignable<UniqueListIterator>::value);
}

TEST(UniqueListIteratorTest, IsMoveConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_move_constructible<UniqueListIterator>::value);
  ASSERT_TRUE(std::is_move_assignable<UniqueListIterator>::value);
}

TEST(UniqueListIteratorTest, DefaultConstructedHasProperState) {
  ASSERT_THAT(UniqueListIterator().available(), Eq(0));
  ASSERT_THAT(UniqueListIterator().hasNext(), Eq(false));
  // Calling `next()` or `peekNext()` or `remove()` is undefined behavior.
}

TEST_P(UniqueListIteratorTest, Iterate) {
  UniqueListIterator iter = getIterator();
  ASSERT_THAT(iter.available(), Eq(GetParam()));
  for (std::size_t i = 0; iter.hasNext(); ++i) {
    ASSERT_THAT(iter.available(), Eq(GetParam() - i));
    ASSERT_THAT(iter.next(), Eq(std::to_string(i)));
  }
  ASSERT_THAT(iter.hasNext(), Eq(false));
  ASSERT_THAT(iter.available(), Eq(0));
}

TEST_P(UniqueListIteratorTest, IterateOnceAndRemoveEvery23thValue) {
  UniqueListIterator iter = getIterator();
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

TEST_P(UniqueListIteratorTest, IterateTwiceAndRemoveEvery23thValueIn1stRun) {
  std::size_t num_removed = 0;
  {
    UniqueListIterator iter = getIterator();
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
    UniqueListIterator iter = getIterator();
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

INSTANTIATE_TEST_CASE_P(Parameterized, UniqueListIteratorTest,
                        testing::Values(0, 1, 2, 10, 100, 1000, 1000000));

} // namespace internal
} // namespace multimap
