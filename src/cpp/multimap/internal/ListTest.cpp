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
#include "multimap/internal/Generator.hpp"
#include "multimap/internal/List.hpp"

namespace multimap {
namespace internal {

using testing::Eq;

// -----------------------------------------------------------------------------
// class List::Iterator
// -----------------------------------------------------------------------------

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
// -----------------------------------------------------------------------------

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
// -----------------------------------------------------------------------------

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
  ASSERT_THAT(List().head().num_values_total, Eq(0));
  ASSERT_THAT(List().size(), Eq(0));
  ASSERT_THAT(List().empty(), Eq(true));
  ASSERT_THAT(List().is_locked(), Eq(false));
}

// -----------------------------------------------------------------------------
// class List / Iteration
// -----------------------------------------------------------------------------

struct ListTestIteration : testing::TestWithParam<uint32_t> {
  void SetUp() override {
    directory = "/tmp/multimap.ListTestIteration";
    boost::filesystem::remove_all(directory);
    MT_ASSERT_TRUE(boost::filesystem::create_directory(directory));

    store.reset(new Store(directory / "store", Store::Options()));
  }

  void TearDown() override {
    store.reset(); // Destructor flushes all data to disk.
    MT_ASSERT_TRUE(boost::filesystem::remove_all(directory));
  }

  Store* getStore() { return store.get(); }
  Arena* getArena() { return &arena; }

private:
  boost::filesystem::path directory;
  std::unique_ptr<Store> store;
  Arena arena;
};

TEST_P(ListTestIteration, AddSmallValuesAndIterateOnce) {
  List list;
  for (size_t i = 0; i != GetParam(); ++i) {
    const auto value = std::to_string(i);
    list.add(value, getStore(), getArena());
    ASSERT_EQ(list.head().num_values_removed, 0);
    ASSERT_EQ(list.head().num_values_total, i + 1);
  }
  ASSERT_EQ(list.size(), GetParam());

  auto iter = list.iterator(*getStore());
  for (size_t i = 0; i != GetParam(); ++i) {
    ASSERT_TRUE(iter.hasNext());
    ASSERT_EQ(iter.available(), GetParam() - i);
    ASSERT_EQ(iter.next(), std::to_string(i));
  }
  ASSERT_FALSE(iter.hasNext());
  ASSERT_EQ(iter.available(), 0);
}

TEST_P(ListTestIteration, AddSmallValuesAndIterateTwice) {
  List list;
  for (size_t i = 0; i != GetParam(); ++i) {
    const auto value = std::to_string(i);
    list.add(value, getStore(), getArena());
    ASSERT_EQ(list.head().num_values_removed, 0);
    ASSERT_EQ(list.head().num_values_total, i + 1);
  }
  ASSERT_EQ(list.size(), GetParam());

  auto iter = list.iterator(*getStore());
  for (size_t i = 0; i != GetParam(); ++i) {
    ASSERT_TRUE(iter.hasNext());
    ASSERT_EQ(iter.available(), GetParam() - i);
    ASSERT_EQ(iter.next(), std::to_string(i));
  }
  ASSERT_FALSE(iter.hasNext());
  ASSERT_EQ(iter.available(), 0);

  iter = list.iterator(*getStore());
  for (size_t i = 0; i != GetParam(); ++i) {
    ASSERT_TRUE(iter.hasNext());
    ASSERT_EQ(iter.available(), GetParam() - i);
    ASSERT_EQ(iter.next(), std::to_string(i));
  }
  ASSERT_FALSE(iter.hasNext());
  ASSERT_EQ(iter.available(), 0);
}

TEST_P(ListTestIteration, AddLargeValuesAndIterateOnce) {
  List list;
  SequenceGenerator gen;
  const size_t size = getStore()->getBlockSize() * 2.5;
  for (size_t i = 0; i != GetParam(); ++i) {
    list.add(gen.generate(size), getStore(), getArena());
    ASSERT_EQ(list.head().num_values_removed, 0);
    ASSERT_EQ(list.head().num_values_total, i + 1);
  }
  ASSERT_EQ(list.size(), GetParam());

  gen.reset();
  auto iter = list.iterator(*getStore());
  for (size_t i = 0; i != GetParam(); ++i) {
    ASSERT_TRUE(iter.hasNext());
    ASSERT_EQ(iter.available(), GetParam() - i);
    ASSERT_EQ(iter.next(), gen.generate(size));
  }
  ASSERT_FALSE(iter.hasNext());
  ASSERT_EQ(iter.available(), 0);
}

TEST_P(ListTestIteration, AddLargeValuesAndIterateTwice) {
  List list;
  SequenceGenerator gen;
  const size_t size = getStore()->getBlockSize() * 2.5;
  for (size_t i = 0; i != GetParam(); ++i) {
    list.add(gen.generate(size), getStore(), getArena());
    ASSERT_EQ(list.head().num_values_removed, 0);
    ASSERT_EQ(list.head().num_values_total, i + 1);
  }
  ASSERT_EQ(list.size(), GetParam());

  gen.reset();
  auto iter = list.iterator(*getStore());
  for (size_t i = 0; i != GetParam(); ++i) {
    ASSERT_TRUE(iter.hasNext());
    ASSERT_EQ(iter.available(), GetParam() - i);
    ASSERT_EQ(iter.next(), gen.generate(size));
  }
  ASSERT_FALSE(iter.hasNext());
  ASSERT_EQ(iter.available(), 0);

  gen.reset();
  iter = list.iterator(*getStore());
  for (size_t i = 0; i != GetParam(); ++i) {
    ASSERT_TRUE(iter.hasNext());
    ASSERT_EQ(iter.available(), GetParam() - i);
    ASSERT_EQ(iter.next(), gen.generate(size));
  }
  ASSERT_FALSE(iter.hasNext());
  ASSERT_EQ(iter.available(), 0);
}

TEST_P(ListTestIteration, FlushValuesBetweenAddingThemAndIterate) {
  List list;
  const auto part_size = 1 + GetParam() / 5;
  for (size_t i = 0; i != GetParam(); ++i) {
    list.add(std::to_string(i), getStore(), getArena());
    if (list.size() % part_size == 0) {
      list.flush(getStore());
    }
  }

  auto iter = list.iterator(getStore());
  for (size_t i = 0; i != GetParam(); ++i) {
    ASSERT_TRUE(iter.hasNext());
    ASSERT_THAT(iter.next(), Eq(std::to_string(i)));
  }
}

INSTANTIATE_TEST_CASE_P(Parameterized, ListTestIteration,
                        testing::Values(0, 1, 2, 10, 100, 1000, 1000000));

// -----------------------------------------------------------------------------
// class List / Locking
// -----------------------------------------------------------------------------

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
// class SharedList
// -----------------------------------------------------------------------------

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
  ASSERT_FALSE(SharedList());
}

// -----------------------------------------------------------------------------
// class UniqueList
// -----------------------------------------------------------------------------

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
  ASSERT_FALSE(UniqueList());
}

// -----------------------------------------------------------------------------
// class SharedListIterator
// -----------------------------------------------------------------------------

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

struct ListIteratorTestWithParam : testing::TestWithParam<uint32_t> {
  void SetUp() override {
    boost::filesystem::remove_all(dir);
    MT_ASSERT_TRUE(boost::filesystem::create_directory(dir));

    Store::Options options;
    options.create_if_missing = true;
    store.reset(new Store(boost::filesystem::path(dir) / "store", options));

    for (size_t i = 0; i != GetParam(); ++i) {
      list.add(std::to_string(i), store.get(), &arena);
    }
  }

  void TearDown() override { boost::filesystem::remove_all(dir); }

  const std::string dir = "/tmp/multimap.ListIteratorTestWithParam";
  const std::string key = "key";
  std::unique_ptr<Store> store;
  Arena arena;
  List list;
};

struct SharedListIteratorTestWithParam : public ListIteratorTestWithParam {
  SharedListIterator getIterator() {
    return SharedListIterator(SharedList(list, *store));
  }
};

TEST_P(SharedListIteratorTestWithParam, Iterate) {
  SharedListIterator iter = getIterator();
  ASSERT_EQ(iter.available(), GetParam());
  for (size_t i = 0; iter.hasNext(); ++i) {
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
// -----------------------------------------------------------------------------

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
  UniqueListIterator getIterator() {
    return UniqueListIterator(UniqueList(&list, store.get(), &arena));
  }
};

TEST_P(UniqueListIteratorTestWithParam, Iterate) {
  UniqueListIterator iter = getIterator();
  ASSERT_EQ(iter.available(), GetParam());
  for (size_t i = 0; iter.hasNext(); ++i) {
    ASSERT_EQ(iter.available(), GetParam() - i);
    ASSERT_EQ(iter.next(), std::to_string(i));
  }
  ASSERT_EQ(iter.available(), 0);
  ASSERT_FALSE(iter.hasNext());
}

TEST_P(UniqueListIteratorTestWithParam, IterateOnceAndRemoveEvery23thValue) {
  UniqueListIterator iter = getIterator();
  size_t num_removed = 0;
  for (size_t i = 0; iter.hasNext(); ++i) {
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
  size_t num_removed = 0;
  {
    UniqueListIterator iter = getIterator();
    for (size_t i = 0; iter.hasNext(); ++i) {
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
    for (size_t i = 0; iter.hasNext(); ++i) {
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
