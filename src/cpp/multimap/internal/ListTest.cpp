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

#include <thread>
#include <boost/filesystem/operations.hpp>
#include <gmock/gmock.h>
#include "multimap/internal/List.hpp"
#include "multimap/internal/Generator.hpp"
#include "multimap/Map.hpp"

namespace multimap {
namespace internal {

using testing::Eq;

// -----------------------------------------------------------------------------
// class List::Iterator

TEST(ListIteratorTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<List::Iterator>::value);
}

TEST(ListIteratorTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<List::Iterator>::value);
  ASSERT_FALSE(std::is_copy_assignable<List::Iterator>::value);
}

TEST(ListIteratorTest, IsMoveConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_move_constructible<List::Iterator>::value);
  ASSERT_TRUE(std::is_move_assignable<List::Iterator>::value);
}

TEST(ListIteratorTest, DefaultConstructedHasProperState) {
  ASSERT_EQ(List::Iterator().available(), 0);
  ASSERT_FALSE(List::Iterator().hasNext());
}

// -----------------------------------------------------------------------------
// class List::MutableIterator

TEST(ListMutableIteratorTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<List::MutableIterator>::value);
}

TEST(ListMutableIteratorTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<List::MutableIterator>::value);
  ASSERT_FALSE(std::is_copy_assignable<List::MutableIterator>::value);
}

TEST(ListMutableIteratorTest, IsMoveConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_move_constructible<List::MutableIterator>::value);
  ASSERT_TRUE(std::is_move_assignable<List::MutableIterator>::value);
}

TEST(ListMutableIteratorTest, DefaultConstructedHasProperState) {
  ASSERT_EQ(List::MutableIterator().available(), 0);
  ASSERT_FALSE(List::MutableIterator().hasNext());
}

// -----------------------------------------------------------------------------
// class List

TEST(ListTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<List>::value);
}

TEST(ListTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<List>::value);
  ASSERT_FALSE(std::is_copy_assignable<List>::value);
}

TEST(ListTest, IsMoveConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_move_constructible<List>::value);
  ASSERT_TRUE(std::is_move_assignable<List>::value);
}

TEST(ListTest, DefaultConstructedHasProperState) {
  ASSERT_THAT(List().head().block_ids.empty(), Eq(true));
  ASSERT_THAT(List().head().num_values_removed, Eq(0));
  ASSERT_THAT(List().head().num_values_added, Eq(0));
  ASSERT_THAT(List().size(), Eq(0));
  ASSERT_THAT(List().empty(), Eq(true));
  ASSERT_THAT(List().is_locked(), Eq(false));
}

// -----------------------------------------------------------------------------
// class List / Locking

TEST(ListTest, LockUniqueFailsIfAlreadyLockedUnique) {
  List list;
  list.lock();
  ASSERT_TRUE(list.is_locked());

  bool other_thread_acquired_unique_lock = false;
  std::thread([&] {
                list.lock();
                other_thread_acquired_unique_lock = true;
              }).detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(other_thread_acquired_unique_lock);

  list.unlock();
  // Other thread acquires unique lock now.
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_TRUE(other_thread_acquired_unique_lock);
  ASSERT_TRUE(list.is_locked());
}

TEST(ListTest, LockSharedFailsIfAlreadyLockedUnique) {
  List list;
  list.lock();
  ASSERT_TRUE(list.is_locked());

  bool other_thread_acquired_shared_lock = false;
  std::thread([&] {
                list.lock_shared();
                other_thread_acquired_shared_lock = true;
              }).detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(other_thread_acquired_shared_lock);

  list.unlock();
  // Other thread acquires shared lock now.
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_TRUE(other_thread_acquired_shared_lock);
  ASSERT_TRUE(list.is_locked());
}

TEST(ListTest, LockUniqueFailsIfAlreadyLockedShared) {
  List list;
  list.lock_shared();
  ASSERT_TRUE(list.is_locked());

  bool other_thread_acquired_unique_lock = false;
  std::thread([&] {
                list.lock();
                other_thread_acquired_unique_lock = true;
              }).detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_FALSE(other_thread_acquired_unique_lock);

  list.unlock_shared();
  // Other thread acquires unique lock now.
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_TRUE(other_thread_acquired_unique_lock);
  ASSERT_TRUE(list.is_locked());
}

TEST(ListTest, LockSharedSucceedsIfAlreadyLockedShared) {
  List list;
  list.lock_shared();
  ASSERT_TRUE(list.is_locked());

  bool other_thread_acquired_shared_lock = false;
  std::thread([&] {
                list.lock_shared();
                other_thread_acquired_shared_lock = true;
              }).detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_TRUE(other_thread_acquired_shared_lock);

  list.unlock_shared();
  // Other thread still owns shared lock.
  ASSERT_TRUE(list.is_locked());

  list.unlock_shared();
  ASSERT_FALSE(list.is_locked());
}

TEST(ListTest, TryLockUniqueFailsIfAlreadyLockedUnique) {
  List list;
  list.lock();
  ASSERT_TRUE(list.is_locked());
  std::thread([&] { ASSERT_FALSE(list.try_lock()); }).join();
}

TEST(ListTest, TryLockSharedFailsIfAlreadyLockedUnique) {
  List list;
  list.lock();
  ASSERT_TRUE(list.is_locked());
  std::thread([&] { ASSERT_FALSE(list.try_lock_shared()); }).join();
}

TEST(ListTest, TryLockUniqueFailsIfAlreadyLockedShared) {
  List list;
  list.lock_shared();
  ASSERT_TRUE(list.is_locked());
  std::thread([&] { ASSERT_FALSE(list.try_lock()); }).join();
}

TEST(ListTest, TryLockSharedSucceedsIfAlreadyLockedShared) {
  List list;
  list.lock_shared();
  ASSERT_TRUE(list.is_locked());
  std::thread([&] { ASSERT_TRUE(list.try_lock_shared()); }).join();
}

// -----------------------------------------------------------------------------
// class List / Iteration

struct ListTestIteration : testing::TestWithParam<std::uint32_t> {
  void SetUp() override {
    directory = "/tmp/multimap.ListTestIteration";
    boost::filesystem::remove_all(directory);
    assert(boost::filesystem::create_directory(directory));

    Store::Options store_opts;
    store_opts.block_size = 128;
    store_opts.create_if_missing = true;
    store_opts.error_if_exists = true;
    store.reset(new Store(directory / "store", store_opts));
  }

  void TearDown() override {
    store.reset(); // Destructor flushes all data to disk.
    assert(boost::filesystem::remove_all(directory));
  }

  Store* getStore() { return store.get(); }
  Arena* getArena() { return &arena; }

private:
  Arena arena;
  std::unique_ptr<Store> store;
  boost::filesystem::path directory;
};

TEST_P(ListTestIteration, AddSmallValuesAndIterateOnce) {
  List list;
  for (std::size_t i = 0; i != GetParam(); ++i) {
    const auto value = std::to_string(i);
    list.add(value, getStore(), getArena());
    ASSERT_EQ(list.head().num_values_removed, 0);
    ASSERT_EQ(list.head().num_values_added, i + 1);
  }
  ASSERT_EQ(list.size(), GetParam());

  auto iter = list.iterator(*getStore());
  for (std::size_t i = 0; i != GetParam(); ++i) {
    ASSERT_TRUE(iter.hasNext());
    ASSERT_EQ(iter.available(), GetParam() - i);
    ASSERT_EQ(iter.next().toString(), std::to_string(i));
  }
  ASSERT_FALSE(iter.hasNext());
  ASSERT_EQ(iter.available(), 0);
}

TEST_P(ListTestIteration, AddSmallValuesAndIterateTwice) {
  List list;
  for (std::size_t i = 0; i != GetParam(); ++i) {
    const auto value = std::to_string(i);
    list.add(value, getStore(), getArena());
    ASSERT_EQ(list.head().num_values_removed, 0);
    ASSERT_EQ(list.head().num_values_added, i + 1);
  }
  ASSERT_EQ(list.size(), GetParam());

  auto iter = list.iterator(*getStore());
  for (std::size_t i = 0; i != GetParam(); ++i) {
    ASSERT_TRUE(iter.hasNext());
    ASSERT_EQ(iter.available(), GetParam() - i);
    ASSERT_EQ(iter.next().toString(), std::to_string(i));
  }
  ASSERT_FALSE(iter.hasNext());
  ASSERT_EQ(iter.available(), 0);

  iter = list.iterator(*getStore());
  for (std::size_t i = 0; i != GetParam(); ++i) {
    ASSERT_TRUE(iter.hasNext());
    ASSERT_EQ(iter.available(), GetParam() - i);
    ASSERT_EQ(iter.next().toString(), std::to_string(i));
  }
  ASSERT_FALSE(iter.hasNext());
  ASSERT_EQ(iter.available(), 0);
}

TEST_P(ListTestIteration, AddLargeValuesAndIterateOnce) {
  List list;
  SequenceGenerator gen;
  const std::size_t size = getStore()->getBlockSize() * 2.5;
  for (std::size_t i = 0; i != GetParam(); ++i) {
    list.add(gen.generate(size), getStore(), getArena());
    ASSERT_EQ(list.head().num_values_removed, 0);
    ASSERT_EQ(list.head().num_values_added, i + 1);
  }
  ASSERT_EQ(list.size(), GetParam());

  gen.reset();
  auto iter = list.iterator(*getStore());
  for (std::size_t i = 0; i != GetParam(); ++i) {
    ASSERT_TRUE(iter.hasNext());
    ASSERT_EQ(iter.available(), GetParam() - i);
    ASSERT_EQ(iter.next().toString(), gen.generate(size));
  }
  ASSERT_FALSE(iter.hasNext());
  ASSERT_EQ(iter.available(), 0);
}

TEST_P(ListTestIteration, AddLargeValuesAndIterateTwice) {
  List list;
  SequenceGenerator gen;
  const std::size_t size = getStore()->getBlockSize() * 2.5;
  for (std::size_t i = 0; i != GetParam(); ++i) {
    list.add(gen.generate(size), getStore(), getArena());
    ASSERT_EQ(list.head().num_values_removed, 0);
    ASSERT_EQ(list.head().num_values_added, i + 1);
  }
  ASSERT_EQ(list.size(), GetParam());

  gen.reset();
  auto iter = list.iterator(*getStore());
  for (std::size_t i = 0; i != GetParam(); ++i) {
    ASSERT_TRUE(iter.hasNext());
    ASSERT_EQ(iter.available(), GetParam() - i);
    ASSERT_EQ(iter.next().toString(), gen.generate(size));
  }
  ASSERT_FALSE(iter.hasNext());
  ASSERT_EQ(iter.available(), 0);

  gen.reset();
  iter = list.iterator(*getStore());
  for (std::size_t i = 0; i != GetParam(); ++i) {
    ASSERT_TRUE(iter.hasNext());
    ASSERT_EQ(iter.available(), GetParam() - i);
    ASSERT_EQ(iter.next().toString(), gen.generate(size));
  }
  ASSERT_FALSE(iter.hasNext());
  ASSERT_EQ(iter.available(), 0);
}

INSTANTIATE_TEST_CASE_P(Parameterized, ListTestIteration,
                        testing::Values(0, 1, 2, 10, 100, 1000, 1000000));

// -----------------------------------------------------------------------------
// class SharedList

TEST(SharedListTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<SharedList>::value);
}

TEST(SharedListTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<SharedList>::value);
  ASSERT_FALSE(std::is_copy_assignable<SharedList>::value);
}

TEST(SharedListTest, IsMoveConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_move_constructible<SharedList>::value);
  ASSERT_TRUE(std::is_move_assignable<SharedList>::value);
}

TEST(SharedListTest, DefaultConstructedHasProperState) {
  ASSERT_TRUE(SharedList().isNull());
}

// -----------------------------------------------------------------------------
// class UniqueList

TEST(UniqueListTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<UniqueList>::value);
}

TEST(UniqueListTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<UniqueList>::value);
  ASSERT_FALSE(std::is_copy_assignable<UniqueList>::value);
}

TEST(UniqueListTest, IsMoveConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_move_constructible<UniqueList>::value);
  ASSERT_TRUE(std::is_move_assignable<UniqueList>::value);
}

TEST(UniqueListTest, DefaultConstructedHasProperState) {
  ASSERT_TRUE(UniqueList().isNull());
}

// -----------------------------------------------------------------------------
// class SharedListIterator

TEST(SharedListIteratorTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<SharedListIterator>::value);
}

TEST(SharedListIteratorTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<SharedListIterator>::value);
  ASSERT_FALSE(std::is_copy_assignable<SharedListIterator>::value);
}

TEST(SharedListIteratorTest, IsMoveConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_move_constructible<SharedListIterator>::value);
  ASSERT_TRUE(std::is_move_assignable<SharedListIterator>::value);
}

TEST(SharedListIteratorTest, DefaultConstructedHasProperState) {
  ASSERT_EQ(SharedListIterator().available(), 0);
  ASSERT_FALSE(SharedListIterator().hasNext());
  // Calling `next()` or `peekNext()` or `remove()` is undefined behavior.
}

struct ListIteratorTestWithParam : testing::TestWithParam<std::uint32_t> {
  void SetUp() override {
    directory = "/tmp/multimap.ListIteratorTestWithParam";
    boost::filesystem::remove_all(directory);
    assert(boost::filesystem::create_directory(directory));

    // Replace `Map` by `Table`.
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

struct SharedListIteratorTestWithParam : public ListIteratorTestWithParam {
  SharedListIterator getIterator() { return map.get(key); }
};

TEST_P(SharedListIteratorTestWithParam, Iterate) {
  SharedListIterator iter = getIterator();
  ASSERT_EQ(iter.available(), GetParam());
  for (std::size_t i = 0; iter.hasNext(); ++i) {
    ASSERT_EQ(iter.available(), GetParam() - i);
    ASSERT_EQ(iter.next(), std::to_string(i));
  }
  ASSERT_EQ(iter.available(), 0);
  ASSERT_FALSE(iter.hasNext());
}

INSTANTIATE_TEST_CASE_P(Parameterized, SharedListIteratorTestWithParam,
                        testing::Values(0, 1, 2, 10, 100, 1000, 1000000));

// -----------------------------------------------------------------------------
// class UniqueListIterator

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
  ASSERT_EQ(UniqueListIterator().available(), 0);
  ASSERT_FALSE(UniqueListIterator().hasNext());
  // Calling `next()` or `peekNext()` or `remove()` is undefined behavior.
}

struct UniqueListIteratorTestWithParam : public ListIteratorTestWithParam {
  UniqueListIterator getIterator() { return map.getMutable(key); }
};

TEST_P(UniqueListIteratorTestWithParam, Iterate) {
  UniqueListIterator iter = getIterator();
  ASSERT_EQ(iter.available(), GetParam());
  for (std::size_t i = 0; iter.hasNext(); ++i) {
    ASSERT_EQ(iter.available(), GetParam() - i);
    ASSERT_EQ(iter.next(), std::to_string(i));
  }
  ASSERT_EQ(iter.available(), 0);
  ASSERT_FALSE(iter.hasNext());
}

TEST_P(UniqueListIteratorTestWithParam, IterateOnceAndRemoveEvery23thValue) {
  UniqueListIterator iter = getIterator();
  std::size_t num_removed = 0;
  for (std::size_t i = 0; iter.hasNext(); ++i) {
    ASSERT_EQ(iter.next(), std::to_string(i));
    if (i % 23 == 0) {
      iter.remove();
      ++num_removed;
    }
  }
  ASSERT_EQ(iter.available(), 0);
  ASSERT_FALSE(iter.hasNext());
}

TEST_P(UniqueListIteratorTestWithParam,
       IterateTwiceAndRemoveEvery23thValueIn1stRun) {
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
    ASSERT_EQ(iter.available(), 0);
    ASSERT_FALSE(iter.hasNext());
  }
  {
    UniqueListIterator iter = getIterator();
    ASSERT_EQ(iter.available(), GetParam() - num_removed);
    for (std::size_t i = 0; iter.hasNext(); ++i) {
      if (i % 23 != 0) {
        ASSERT_NE(std::stoi(iter.next().toString()) % 23, 0);
      }
    }
    ASSERT_EQ(iter.available(), 0);
    ASSERT_FALSE(iter.hasNext());
  }
}

INSTANTIATE_TEST_CASE_P(Parameterized, UniqueListIteratorTestWithParam,
                        testing::Values(0, 1, 2, 10, 100, 1000, 1000000));

} // namespace internal
} // namespace multimap
