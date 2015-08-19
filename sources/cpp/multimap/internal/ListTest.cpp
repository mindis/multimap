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
  ASSERT_FALSE(ListIter().HasValue());
}

TEST(ListConstIterTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<ListConstIter>::value);
}

TEST(ListConstIterTest, DefaultConstructedHasProperState) {
  ASSERT_FALSE(ListConstIter().HasValue());
}

TEST(ListTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<List>::value);
}

TEST(ListTest, DefaultConstructedHasProperState) {
  ASSERT_THAT(List().head().block_ids.empty(), Eq(true));
  ASSERT_THAT(List().head().num_values_deleted, Eq(0));
  ASSERT_THAT(List().head().num_values_total, Eq(0));
  ASSERT_THAT(List().block().has_data(), Eq(false));
  ASSERT_THAT(List().size(), Eq(0));
  ASSERT_THAT(List().empty(), Eq(true));
  ASSERT_THAT(List().locked(), Eq(false));
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
  Callbacks callbacks;

  BlockPool block_pool(100000, 128);
  callbacks.allocate_block = [&block_pool]() {
    assert(!block_pool.empty());
    return block_pool.Pop();
  };

  std::vector<Block> blocks;
  callbacks.commit_block = [&blocks](Block&& block) {
    blocks.push_back(std::move(block));
    return blocks.size() - 1;
  };

  List list;
  for (std::size_t i = 0; i != GetParam(); ++i) {
    const auto value = std::to_string(i);
    list.Add(value, callbacks.allocate_block, callbacks.commit_block);
    ASSERT_THAT(list.head().num_values_deleted, Eq(0));
    ASSERT_THAT(list.head().num_values_total, Eq(i + 1));
  }
  ASSERT_THAT(list.size(), Eq(GetParam()));

  callbacks.deallocate_block =
      [&block_pool](Block&& block) { block_pool.Push(std::move(block)); };

  callbacks.request_block =
      [&blocks](std::uint32_t id, Block* block, Arena* arena) {
    assert(id < blocks.size());

    if (!block->has_data()) {
      const auto block_size = blocks[id].size();
      assert(block_size == 128);
      block->set_data(arena->Allocate(block_size), block_size);
    }
    assert(block->size() == 128);
    std::memcpy(block->data(), blocks[id].data(), block->size());
  };

  auto iter = list.NewConstIterator(callbacks);
  iter.SeekToFirst();
  for (std::size_t i = 0; i != GetParam(); ++i) {
    ASSERT_THAT(iter.HasValue(), Eq(true));
    ASSERT_THAT(iter.GetValue().ToString(), Eq(std::to_string(i)));
    iter.Next();
  }
  ASSERT_THAT(iter.HasValue(), Eq(false));

  std::size_t i = 0;
  for (iter.SeekToFirst(); iter.HasValue(); iter.Next(), ++i) {
    ASSERT_THAT(iter.GetValue().ToString(), Eq(std::to_string(i)));
  }
  ASSERT_THAT(i, Eq(GetParam()));
}

INSTANTIATE_TEST_CASE_P(Parameterized, ListTestWithParam,
                        testing::Values(0, 1, 2, 10, 100, 1000, 1000000));

TEST(ListTest, LockUniqueFailsIfAlreadyLockedUnique) {
  List list;
  list.LockUnique();
  ASSERT_THAT(list.locked(), Eq(true));

  bool other_thread_gots_unique_lock = false;
  std::thread thread([&] {
    list.LockUnique();
    other_thread_gots_unique_lock = true;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_THAT(other_thread_gots_unique_lock, Eq(false));

  // Release lock from 1st thread.
  ASSERT_THAT(list.locked(), Eq(true));
  list.UnlockUnique();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_THAT(other_thread_gots_unique_lock, Eq(true));

  // Release lock from 2nd thread.
  ASSERT_THAT(list.locked(), Eq(true));
  list.UnlockUnique();
  ASSERT_THAT(list.locked(), Eq(false));
}

TEST(ListTest, UniqueLockFailsIfAlreadyLockedShared) {
  List list;
  list.LockShared();
  ASSERT_THAT(list.locked(), Eq(true));

  bool other_thread_gots_unique_lock = false;
  std::thread thread([&] {
    list.LockUnique();
    other_thread_gots_unique_lock = true;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_THAT(other_thread_gots_unique_lock, Eq(false));

  // Release lock from 1st thread.
  ASSERT_THAT(list.locked(), Eq(true));
  list.UnlockShared();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_THAT(other_thread_gots_unique_lock, Eq(true));

  // Release lock from 2nd thread.
  ASSERT_THAT(list.locked(), Eq(true));
  list.UnlockUnique();
  ASSERT_THAT(list.locked(), Eq(false));
}

TEST(ListTest, SharedLockFailsIfAlreadyLockedUnique) {
  List list;
  list.LockUnique();
  ASSERT_THAT(list.locked(), Eq(true));

  bool other_thread_gots_shared_lock = false;
  std::thread thread([&] {
    list.LockShared();
    other_thread_gots_shared_lock = true;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_THAT(other_thread_gots_shared_lock, Eq(false));

  // Release lock from 1st thread.
  ASSERT_THAT(list.locked(), Eq(true));
  list.UnlockUnique();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_THAT(other_thread_gots_shared_lock, Eq(true));

  // Release lock from 2nd thread.
  ASSERT_THAT(list.locked(), Eq(true));
  list.UnlockShared();
  ASSERT_THAT(list.locked(), Eq(false));
}

TEST(ListTest, LockSharedDoesNotFailIfAlreadyLockedShared) {
  List list;
  list.LockShared();
  ASSERT_THAT(list.locked(), Eq(true));

  bool other_thread_gots_shared_lock = false;
  std::thread thread([&] {
    list.LockShared();
    other_thread_gots_shared_lock = true;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_THAT(other_thread_gots_shared_lock, Eq(true));

  // Release lock from 1st thread.
  ASSERT_THAT(list.locked(), Eq(true));
  list.UnlockShared();
  ASSERT_THAT(list.locked(), Eq(true));

  // Release lock from 2nd thread.
  ASSERT_THAT(list.locked(), Eq(true));
  list.UnlockShared();
  ASSERT_THAT(list.locked(), Eq(false));
}

TEST(ListTest, TryLockUniqueFailsIfAlreadyLockedShared) {
  List list;
  list.LockShared();
  ASSERT_THAT(list.locked(), Eq(true));

  std::thread thread([&] { ASSERT_THAT(list.TryLockUnique(), Eq(false)); });
  thread.join();
}

TEST(ListTest, TryLockSharedFailsIfAlreadyLockedUnique) {
  List list;
  list.LockUnique();
  ASSERT_THAT(list.locked(), Eq(true));

  std::thread thread([&] { ASSERT_THAT(list.TryLockShared(), Eq(false)); });
  thread.join();
}

TEST(ListTest, TryLockSharedDoesNotFailIfAlreadyLockedShared) {
  List list;
  list.LockShared();
  ASSERT_THAT(list.locked(), Eq(true));

  std::thread thread([&] { ASSERT_THAT(list.TryLockShared(), Eq(true)); });
  thread.join();
}

}  // namespace internal
}  // namespace multimap
