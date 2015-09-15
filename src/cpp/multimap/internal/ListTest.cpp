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
#include "gmock/gmock.h"
#include "multimap/internal/List.hpp"
#include "multimap/internal/BlockPool.hpp"

namespace multimap {
namespace internal {

using testing::Eq;

typedef List::Iterator ListIter;
typedef List::ConstIterator ListConstIter;

TEST(ListIterTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<ListIter>::value);
}

TEST(ListIterTest, DefaultConstructedHasProperState) {
  ASSERT_FALSE(ListIter().hasValue());
}

TEST(ListConstIterTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<ListConstIter>::value);
}

TEST(ListConstIterTest, DefaultConstructedHasProperState) {
  ASSERT_FALSE(ListConstIter().hasValue());
}

TEST(ListTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<List>::value);
}

TEST(ListTest, DefaultConstructedHasProperState) {
  ASSERT_THAT(List().head().block_ids.empty(), Eq(true));
  ASSERT_THAT(List().head().num_values_deleted, Eq(0));
  ASSERT_THAT(List().head().num_values_total, Eq(0));
  ASSERT_THAT(List().block().hasData(), Eq(false));
  ASSERT_THAT(List().size(), Eq(0));
  ASSERT_THAT(List().empty(), Eq(true));
  ASSERT_THAT(List().isLocked(), Eq(false));
}

TEST(ListTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<List>::value);
  ASSERT_FALSE(std::is_copy_assignable<List>::value);
}

TEST(ListTest, IsNotMoveConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_move_constructible<List>::value);
  ASSERT_FALSE(std::is_move_assignable<List>::value);
}

struct ListTestWithParam : testing::TestWithParam<std::uint32_t> {};

TEST_P(ListTestWithParam, AddValuesAndIterateAll) {
  BlockPool block_pool(128);
  const auto allocate_block_callback =
      [&block_pool]() { return block_pool.allocate(); };

  std::vector<std::string> store;
  const auto commit_block_callback = [&store](const Block& block) {
    store.push_back(std::string(block.data(), block.size()));
    return store.size() - 1;
  };

  List list;
  for (std::size_t i = 0; i != GetParam(); ++i) {
    const auto value = std::to_string(i);
    list.add(value, allocate_block_callback, commit_block_callback);
    ASSERT_THAT(list.head().num_values_deleted, Eq(0));
    ASSERT_THAT(list.head().num_values_total, Eq(i + 1));
  }
  ASSERT_THAT(list.size(), Eq(GetParam()));

  const auto request_blocks_callback =
      [&store](std::vector<BlockWithId>* blocks, Arena* arena) {
    assert(blocks);
    for (auto& block : *blocks) {
      assert(block.id < store.size());
      if (!block.hasData()) {
        assert(arena);
        const auto block_size = store[block.id].size();
        block.setData(arena->allocate(block_size), block_size);
      }
      std::memcpy(block.data(), store[block.id].data(), block.size());
    }
  };

  auto iter = list.const_iterator(request_blocks_callback);
  iter.seekToFirst();
  for (std::size_t i = 0; i != GetParam(); ++i) {
    ASSERT_THAT(iter.hasValue(), Eq(true));
    ASSERT_THAT(iter.getValue().toString(), Eq(std::to_string(i)));
    iter.next();
  }
  ASSERT_THAT(iter.hasValue(), Eq(false));

  std::size_t i = 0;
  for (iter.seekToFirst(); iter.hasValue(); iter.next(), ++i) {
    ASSERT_THAT(iter.getValue().toString(), Eq(std::to_string(i)));
  }
  ASSERT_THAT(i, Eq(GetParam()));
}

INSTANTIATE_TEST_CASE_P(Parameterized, ListTestWithParam,
                        testing::Values(0, 1, 2, 10, 100, 1000, 1000000));

TEST(ListTest, LockUniqueFailsIfAlreadyLockedUnique) {
  List list;
  list.lockUnique();
  ASSERT_THAT(list.isLocked(), Eq(true));

  bool other_thread_gots_unique_lock = false;
  std::thread thread([&] {
    list.lockUnique();
    other_thread_gots_unique_lock = true;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_THAT(other_thread_gots_unique_lock, Eq(false));

  // Release lock from 1st thread.
  ASSERT_THAT(list.isLocked(), Eq(true));
  list.unlockUnique();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_THAT(other_thread_gots_unique_lock, Eq(true));

  // Release lock from 2nd thread.
  ASSERT_THAT(list.isLocked(), Eq(true));
  list.unlockUnique();
  ASSERT_THAT(list.isLocked(), Eq(false));
}

TEST(ListTest, UniqueLockFailsIfAlreadyLockedShared) {
  List list;
  list.lockShared();
  ASSERT_THAT(list.isLocked(), Eq(true));

  bool other_thread_gots_unique_lock = false;
  std::thread thread([&] {
    list.lockUnique();
    other_thread_gots_unique_lock = true;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_THAT(other_thread_gots_unique_lock, Eq(false));

  // Release lock from 1st thread.
  ASSERT_THAT(list.isLocked(), Eq(true));
  list.unlockShared();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_THAT(other_thread_gots_unique_lock, Eq(true));

  // Release lock from 2nd thread.
  ASSERT_THAT(list.isLocked(), Eq(true));
  list.unlockUnique();
  ASSERT_THAT(list.isLocked(), Eq(false));
}

TEST(ListTest, SharedLockFailsIfAlreadyLockedUnique) {
  List list;
  list.lockUnique();
  ASSERT_THAT(list.isLocked(), Eq(true));

  bool other_thread_gots_shared_lock = false;
  std::thread thread([&] {
    list.lockShared();
    other_thread_gots_shared_lock = true;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_THAT(other_thread_gots_shared_lock, Eq(false));

  // Release lock from 1st thread.
  ASSERT_THAT(list.isLocked(), Eq(true));
  list.unlockUnique();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_THAT(other_thread_gots_shared_lock, Eq(true));

  // Release lock from 2nd thread.
  ASSERT_THAT(list.isLocked(), Eq(true));
  list.unlockShared();
  ASSERT_THAT(list.isLocked(), Eq(false));
}

TEST(ListTest, LockSharedDoesNotFailIfAlreadyLockedShared) {
  List list;
  list.lockShared();
  ASSERT_THAT(list.isLocked(), Eq(true));

  bool other_thread_gots_shared_lock = false;
  std::thread thread([&] {
    list.lockShared();
    other_thread_gots_shared_lock = true;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_THAT(other_thread_gots_shared_lock, Eq(true));

  // Release lock from 1st thread.
  ASSERT_THAT(list.isLocked(), Eq(true));
  list.unlockShared();
  ASSERT_THAT(list.isLocked(), Eq(true));

  // Release lock from 2nd thread.
  ASSERT_THAT(list.isLocked(), Eq(true));
  list.unlockShared();
  ASSERT_THAT(list.isLocked(), Eq(false));
}

TEST(ListTest, TryLockUniqueFailsIfAlreadyLockedShared) {
  List list;
  list.lockShared();
  ASSERT_THAT(list.isLocked(), Eq(true));

  std::thread thread([&] { ASSERT_THAT(list.tryLockUnique(), Eq(false)); });
  thread.join();
}

TEST(ListTest, TryLockSharedFailsIfAlreadyLockedUnique) {
  List list;
  list.lockUnique();
  ASSERT_THAT(list.isLocked(), Eq(true));

  std::thread thread([&] { ASSERT_THAT(list.tryLockShared(), Eq(false)); });
  thread.join();
}

TEST(ListTest, TryLockSharedDoesNotFailIfAlreadyLockedShared) {
  List list;
  list.lockShared();
  ASSERT_THAT(list.isLocked(), Eq(true));

  std::thread thread([&] { ASSERT_THAT(list.tryLockShared(), Eq(true)); });
  thread.join();
}

}  // namespace internal
}  // namespace multimap
