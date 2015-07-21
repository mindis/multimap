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

struct TableTest : public testing::Test {
  static void SetUpTestCase() {
    filepath = "/tmp/multimap-TableTest.table";
    block_cache_size = 1024 * 1024;
    block_size = 128;
    key1 = Bytes("k1");
    key2 = Bytes("k2");
    key3 = Bytes("k3");
  }

  //  void SetUp() override { boost::filesystem::create_directory(table_file); }

  void TearDown() override { boost::filesystem::remove(filepath); }

  static boost::filesystem::path filepath;
  static std::size_t block_cache_size;
  static std::size_t block_size;
  static Bytes key1;
  static Bytes key2;
  static Bytes key3;
};

boost::filesystem::path TableTest::filepath;
std::size_t TableTest::block_cache_size;
std::size_t TableTest::block_size;
Bytes TableTest::key1;
Bytes TableTest::key2;
Bytes TableTest::key3;

TEST_F(TableTest, IsDefaultConstructible) {
  ASSERT_THAT(std::is_default_constructible<Table>::value, Eq(true));
}

TEST_F(TableTest, DefaultConstructedState) {
  for (const auto& entry : Table().GetProperties()) {
    ASSERT_THAT(entry.second, Eq("0"));
  }
}

TEST_F(TableTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_THAT(std::is_copy_constructible<Table>::value, Eq(false));
  ASSERT_THAT(std::is_copy_assignable<Table>::value, Eq(false));
}

TEST_F(TableTest, IsNotMoveConstructibleOrAssignable) {
  ASSERT_THAT(std::is_move_constructible<Table>::value, Eq(false));
  ASSERT_THAT(std::is_move_assignable<Table>::value, Eq(false));
  // Cannot be moved because of mutex member.
}

TEST_F(TableTest, DefaultConstructedGetShared) {
  ASSERT_THAT(Table().GetShared(key1).clist(), IsNull());
}

TEST_F(TableTest, DefaultConstructedGetUnique) {
  ASSERT_THAT(Table().GetUnique(key1).clist(), IsNull());
}

TEST_F(TableTest, DefaultConstructedGetUniqueOrCreate) {
  ASSERT_THAT(Table().GetUniqueOrCreate(key1).clist(), NotNull());
}

TEST_F(TableTest, DefaultConstructedDelete) {
  ASSERT_THAT(Table().Delete(key1), Eq(false));
}

TEST_F(TableTest, DefaultConstructedContains) {
  ASSERT_THAT(Table().Contains(key1), Eq(false));
}

TEST_F(TableTest, DefaultConstructedForEachKey) {
  Table::KeyProcedure procedure = [](const Bytes& /* key */) {};
  ASSERT_NO_THROW(Table().ForEachKey(procedure));
}

TEST_F(TableTest, DefaultConstructedForEachList) {
  Table::ListProcedure procedure = [](const List& /* list */) {};
  ASSERT_NO_THROW(Table().ForEachList(procedure));
}

// TEST_F(TableTest, OpenInNonExistingDirectory) {
//  boost::filesystem::remove_all(directory);

//  Table table;
//  ASSERT_ANY_THROW(table.Init(directory, block_cache_size, block_size));
//}

// TEST_F(TableTest, OpenInNonEmptyDirectoryWithMissingFiles) {
//  boost::filesystem::create_directories(directory / "dummy");

//  Table table;
//  ASSERT_ANY_THROW(table.Init(directory, block_cache_size, block_size));
//}

// TEST_F(TableTest, OpenInEmptyDirectory) {
//  Table table;
//  ASSERT_NO_THROW(table.Init(directory, block_cache_size, block_size));
//}

// TEST_F(TableTest, OpenInEmptyDirectoryTwice) {
//  Table table;
//  ASSERT_NO_THROW(table.Init(directory, block_cache_size, block_size));
//  ASSERT_ANY_THROW(table.Init(directory, block_cache_size, block_size));
//}

TEST_F(TableTest, GetUniqueOrCreateInsertsNewKey) {
  Table table;
  table.Create(filepath);

  table.GetUniqueOrCreate(key1);
  ASSERT_THAT(table.GetShared(key1).clist(), NotNull());
  ASSERT_THAT(table.GetUnique(key1).clist(), NotNull());

  table.GetUniqueOrCreate(key2);
  ASSERT_THAT(table.GetShared(key2).clist(), NotNull());
  ASSERT_THAT(table.GetUnique(key2).clist(), NotNull());
}

TEST_F(TableTest, InsertKeyThenGetSharedTwice) {
  Table table;
  table.Create(filepath);

  table.GetUniqueOrCreate(key1);
  const auto lock1 = table.GetShared(key1);
  ASSERT_THAT(lock1.clist(), NotNull());
  const auto lock2 = table.GetShared(key1);
  ASSERT_THAT(lock2.clist(), NotNull());
}

TEST_F(TableTest, InsertKeyThenGetUniqueTwice) {
  Table table;
  table.Create(filepath);
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

TEST_F(TableTest, InsertKeyThenGetSharedAndGetUnique) {
  Table table;
  table.Create(filepath);
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

TEST_F(TableTest, InsertKeyThenGetUniqueAndGetShared) {
  Table table;
  //  table.Init(directory, block_cache_size, block_size);
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

TEST_F(TableTest, InsertKeysThenGetAllShared) {
  Table table;
  //  table.Init(directory, block_cache_size, block_size);
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

TEST_F(TableTest, InsertKeysThenGetAllUnique) {
  Table table;
  //  table.Init(directory, block_cache_size, block_size);
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

TEST_F(TableTest, InsertAndDeleteKeys) {
  Table table;
  //  table.Init(directory, block_cache_size, block_size);
  table.GetUniqueOrCreate(key1);
  table.GetUniqueOrCreate(key2);

  ASSERT_THAT(table.Delete(key1), Eq(true));
  ASSERT_THAT(table.GetShared(key1).clist(), IsNull());
  ASSERT_THAT(table.GetUnique(key1).clist(), IsNull());

  ASSERT_THAT(table.Delete(key2), Eq(true));
  ASSERT_THAT(table.GetShared(key2).clist(), IsNull());
  ASSERT_THAT(table.GetUnique(key2).clist(), IsNull());

  ASSERT_THAT(table.Delete(key3), Eq(false));
  ASSERT_THAT(table.GetShared(key3).clist(), IsNull());
  ASSERT_THAT(table.GetUnique(key3).clist(), IsNull());
}

TEST_F(TableTest, ForEachKeyVisitsLockedEntries) {
  Table table;
  //  table.Init(directory, block_cache_size, block_size);
  const auto list_lock1 = table.GetUniqueOrCreate(key1);
  const auto list_lock2 = table.GetUniqueOrCreate(key2);
  const auto list_lock3 = table.GetUniqueOrCreate(key3);

  std::vector<Bytes> keys;
  table.ForEachKey([&keys](const Bytes& key) { keys.push_back(key); });
  std::sort(keys.begin(), keys.end());
  ASSERT_THAT(keys, ElementsAre(key1, key2, key3));
}

TEST_F(TableTest, ForEachListDoesNotVisitLockedEntries) {
  Table table;
  //  table.Init(directory, block_cache_size, block_size);
  table.GetUniqueOrCreate(key1);  // Inserted but not locked.
  table.GetUniqueOrCreate(key2);  // Inserted but not locked.
  const auto list_lock = table.GetUniqueOrCreate(key3);

  std::size_t num_visited = 0;
  table.ForEachList([&num_visited](const List&) { ++num_visited; });
  ASSERT_THAT(num_visited, Eq(2));
}

}  // namespace internal
}  // namespace multimap
