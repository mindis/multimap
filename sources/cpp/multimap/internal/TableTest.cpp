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

#include <limits>
#include <sstream>
#include <thread>
#include <type_traits>
#include <boost/filesystem/operations.hpp>
#include "gmock/gmock.h"
#include "multimap/internal/Table.hpp"
#include "multimap/internal/System.hpp"

namespace multimap {
namespace internal {

using testing::Eq;
using testing::IsNull;
using testing::NotNull;
using testing::ElementsAre;
using testing::ElementsAreArray;

struct TableTestFixture : public testing::Test {
  void SetUp() override {
    key1 = Bytes("k1");
    key2 = Bytes("k2");
    key3 = Bytes("k3");
  }

  Bytes key1;
  Bytes key2;
  Bytes key3;
};

TEST_F(TableTestFixture, IsDefaultConstructible) {
  ASSERT_THAT(std::is_default_constructible<Table>::value, Eq(true));
}

TEST_F(TableTestFixture, DefaultConstructedHasProperState) {
  ASSERT_THAT(Table().GetShared(key1).clist(), IsNull());
  ASSERT_THAT(Table().GetUnique(key1).clist(), IsNull());
  ASSERT_THAT(Table().GetUniqueOrCreate(key1).clist(), NotNull());

  std::vector<std::string> keys;
  Callables::Procedure procedure = [&keys](const Bytes& key) {
    keys.push_back(key.ToString());
  };
  Table().ForEachKey(procedure);
  ASSERT_TRUE(keys.empty());

  for (const auto& entry : Table().GetProperties()) {
    ASSERT_THAT(entry.second, Eq("0"));
  }
}

TEST_F(TableTestFixture, IsNotCopyConstructibleOrAssignable) {
  ASSERT_THAT(std::is_copy_constructible<Table>::value, Eq(false));
  ASSERT_THAT(std::is_copy_assignable<Table>::value, Eq(false));
}

TEST_F(TableTestFixture, IsNotMoveConstructibleOrAssignable) {
  ASSERT_THAT(std::is_move_constructible<Table>::value, Eq(false));
  ASSERT_THAT(std::is_move_assignable<Table>::value, Eq(false));
}

TEST_F(TableTestFixture, GetUniqueOrCreateInsertsNewKey) {
  Table table;
  table.GetUniqueOrCreate(key1);
  ASSERT_THAT(table.GetShared(key1).clist(), NotNull());
  ASSERT_THAT(table.GetUnique(key1).clist(), NotNull());

  table.GetUniqueOrCreate(key2);
  ASSERT_THAT(table.GetShared(key2).clist(), NotNull());
  ASSERT_THAT(table.GetUnique(key2).clist(), NotNull());
}

TEST_F(TableTestFixture, InsertKeyThenGetSharedTwice) {
  Table table;
  table.GetUniqueOrCreate(key1);
  const auto lock1 = table.GetShared(key1);
  ASSERT_THAT(lock1.clist(), NotNull());
  const auto lock2 = table.GetShared(key1);
  ASSERT_THAT(lock2.clist(), NotNull());
}

TEST_F(TableTestFixture, InsertKeyThenGetUniqueTwice) {
  Table table;
  table.GetUniqueOrCreate(key1);
  bool other_thread_got_unique_lock = false;
  {
    const auto lock = table.GetUnique(key1);
    ASSERT_THAT(lock.clist(), NotNull());
    // We don't have GetUnique(key, wait_for_some_time)
    // so we start a new thread that tries to acquire the lock.
    std::thread thread([&] {
      table.GetUnique(key1);
      other_thread_got_unique_lock = true;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_THAT(other_thread_got_unique_lock, Eq(false));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_THAT(other_thread_got_unique_lock, Eq(true));
}

TEST_F(TableTestFixture, InsertKeyThenGetSharedAndGetUnique) {
  Table table;
  table.GetUniqueOrCreate(key1);
  bool other_thread_gots_unique_lock = false;
  {
    const auto list_lock = table.GetShared(key1);
    ASSERT_THAT(list_lock.clist(), NotNull());
    // We don't have GetUnique(key, wait_for_some_time)
    // so we start a new thread that tries to acquire the lock.
    std::thread thread([&] {
      table.GetUnique(key1);
      other_thread_gots_unique_lock = true;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_THAT(other_thread_gots_unique_lock, Eq(false));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_THAT(other_thread_gots_unique_lock, Eq(true));
}

TEST_F(TableTestFixture, InsertKeyThenGetUniqueAndGetShared) {
  Table table;
  table.GetUniqueOrCreate(key1);
  bool other_thread_gots_shared_lock = false;
  {
    const auto list_lock = table.GetUnique(key1);
    ASSERT_THAT(list_lock.clist(), NotNull());
    // We don't have GetShared(key, wait_for_some_time)
    // so we start a new thread that tries to acquire the lock.
    std::thread thread([&] {
      table.GetShared(key1);
      other_thread_gots_shared_lock = true;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_THAT(other_thread_gots_shared_lock, Eq(false));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_THAT(other_thread_gots_shared_lock, Eq(true));
}

TEST_F(TableTestFixture, InsertKeysThenGetAllShared) {
  Table table;
  table.GetUniqueOrCreate(key1);
  table.GetUniqueOrCreate(key2);
  bool other_thread_gots_shared_lock = false;

  const auto list_lock = table.GetShared(key1);
  ASSERT_THAT(list_lock.clist(), NotNull());
  // We don't have GetShared(key, wait_for_some_time)
  // so we start a new thread that tries to acquire the lock.
  std::thread thread([&] {
    table.GetShared(key2);
    other_thread_gots_shared_lock = true;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_THAT(other_thread_gots_shared_lock, Eq(true));
}

TEST_F(TableTestFixture, InsertKeysThenGetAllUnique) {
  Table table;
  table.GetUniqueOrCreate(key1);
  table.GetUniqueOrCreate(key2);
  bool other_thread_gots_unique_lock = false;

  const auto list_lock = table.GetUnique(key1);
  ASSERT_THAT(list_lock.clist(), NotNull());
  // We don't have GetUnique(key, wait_for_some_time)
  // so we start a new thread that tries to acquire the lock.
  std::thread thread([&] {
    table.GetUnique(key2);
    other_thread_gots_unique_lock = true;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_THAT(other_thread_gots_unique_lock, Eq(true));
}

TEST_F(TableTestFixture, InsertAndDeleteKeys) {
  Table table;
  table.GetUniqueOrCreate(key1);
  table.GetUniqueOrCreate(key2);

  ASSERT_THAT(table.GetShared(key1).clist(), NotNull());
  ASSERT_THAT(table.GetUnique(key1).clist(), NotNull());

  ASSERT_THAT(table.GetShared(key2).clist(), NotNull());
  ASSERT_THAT(table.GetUnique(key2).clist(), NotNull());

  ASSERT_THAT(table.GetShared(key3).clist(), IsNull());
  ASSERT_THAT(table.GetUnique(key3).clist(), IsNull());
}

TEST_F(TableTestFixture, ForEachKeyIgnoresEmptyLists) {
  Table table;
  const auto list_lock1 = table.GetUniqueOrCreate(key1);
  const auto list_lock2 = table.GetUniqueOrCreate(key2);
  const auto list_lock3 = table.GetUniqueOrCreate(key3);

  std::vector<std::string> keys;
  Callables::Procedure procedure = [&keys](const Bytes& key) {
    keys.push_back(key.ToString());
  };
  Table().ForEachKey(procedure);
  ASSERT_TRUE(keys.empty());
}

}  // namespace internal
}  // namespace multimap
