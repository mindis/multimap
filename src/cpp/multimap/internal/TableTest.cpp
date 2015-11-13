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
#include <type_traits>
#include <gmock/gmock.h>
#include <boost/filesystem/operations.hpp>
#include "multimap/internal/Table.hpp"

namespace multimap {
namespace internal {

TEST(TableTest, IsNotDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<Table>::value);
}

TEST(TableTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<Table>::value);
  ASSERT_FALSE(std::is_copy_assignable<Table>::value);
}

TEST(TableTest, IsNotMoveConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_move_constructible<Table>::value);
  ASSERT_FALSE(std::is_move_assignable<Table>::value);
}

struct TableTestFixture : public testing::Test {
  void SetUp() override {
    directory = "/tmp/multimap.TableTestFixture";
    boost::filesystem::remove_all(directory);
    boost::filesystem::create_directory(directory);
    Table::Options options;
    options.create_if_missing = true;
    table.reset(new Table(directory / "table", options));
    key1 = Bytes("k1");
    key2 = Bytes("k2");
    key3 = Bytes("k3");
  }

  void TearDown() override {
    table.reset(); // Destructor flushes all data to disk.
    boost::filesystem::remove_all(directory);
  }

  boost::filesystem::path directory;
  std::unique_ptr<Table> table;
  Bytes key1;
  Bytes key2;
  Bytes key3;
};

TEST_F(TableTestFixture, GetUniqueOrCreateInsertsNewKey) {
  table->getUniqueOrCreate(key1);
  ASSERT_TRUE(table->getShared(key1));
  ASSERT_TRUE(table->getUnique(key1));

  table->getUniqueOrCreate(key2);
  ASSERT_TRUE(table->getShared(key2));
  ASSERT_TRUE(table->getUnique(key2));
}

TEST_F(TableTestFixture, InsertKeyThenGetSharedTwice) {
  table->getUniqueOrCreate(key1);
  const auto list1 = table->getShared(key1);
  ASSERT_TRUE(list1);
  const auto list2 = table->getShared(key1);
  ASSERT_TRUE(list2);
}

TEST_F(TableTestFixture, InsertKeyThenGetUniqueTwice) {
  table->getUniqueOrCreate(key1);
  bool other_thread_got_unique_list = false;
  {
    const auto list = table->getUnique(key1);
    ASSERT_TRUE(list);
    // We don't have `getUnique(key, wait_for_some_time)` yet,
    // so we start a new thread that tries to acquire the lock.
    std::thread thread([&] {
      table->getUnique(key1);
      other_thread_got_unique_list = true;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_FALSE(other_thread_got_unique_list);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_TRUE(other_thread_got_unique_list);
}

TEST_F(TableTestFixture, InsertKeyThenGetSharedAndGetUnique) {
  table->getUniqueOrCreate(key1);
  bool other_thread_gots_unique_list = false;
  {
    const auto list = table->getShared(key1);
    ASSERT_TRUE(list);
    // We don't have GetUnique(key, wait_for_some_time)
    // so we start a new thread that tries to acquire the lock.
    std::thread thread([&] {
      table->getUnique(key1);
      other_thread_gots_unique_list = true;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_FALSE(other_thread_gots_unique_list);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_TRUE(other_thread_gots_unique_list);
}

TEST_F(TableTestFixture, InsertKeyThenGetUniqueAndGetShared) {
  table->getUniqueOrCreate(key1);
  bool other_thread_gots_shared_list = false;
  {
    const auto list = table->getUnique(key1);
    ASSERT_TRUE(list);
    // We don't have GetShared(key, wait_for_some_time)
    // so we start a new thread that tries to acquire the lock.
    std::thread thread([&] {
      table->getShared(key1);
      other_thread_gots_shared_list = true;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_FALSE(other_thread_gots_shared_list);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_TRUE(other_thread_gots_shared_list);
}

TEST_F(TableTestFixture, InsertKeysThenGetAllShared) {
  table->getUniqueOrCreate(key1);
  table->getUniqueOrCreate(key2);
  bool other_thread_gots_shared_list = false;

  const auto list = table->getShared(key1);
  ASSERT_TRUE(list);
  // We don't have GetShared(key, wait_for_some_time)
  // so we start a new thread that tries to acquire the lock.
  std::thread thread([&] {
    table->getShared(key2);
    other_thread_gots_shared_list = true;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_TRUE(other_thread_gots_shared_list);
}

TEST_F(TableTestFixture, InsertKeysThenGetAllUnique) {
  table->getUniqueOrCreate(key1);
  table->getUniqueOrCreate(key2);
  bool other_thread_gots_unique_list = false;

  const auto list = table->getUnique(key1);
  ASSERT_TRUE(list);
  // We don't have GetUnique(key, wait_for_some_time)
  // so we start a new thread that tries to acquire the lock.
  std::thread thread([&] {
    table->getUnique(key2);
    other_thread_gots_unique_list = true;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_TRUE(other_thread_gots_unique_list);
}

TEST_F(TableTestFixture, InsertAndDeleteKeys) {
  table->getUniqueOrCreate(key1);
  table->getUniqueOrCreate(key2);

  ASSERT_TRUE(table->getShared(key1));
  ASSERT_TRUE(table->getUnique(key1));

  ASSERT_TRUE(table->getShared(key2));
  ASSERT_TRUE(table->getUnique(key2));

  ASSERT_FALSE(table->getShared(key3));
  ASSERT_FALSE(table->getUnique(key3));
}

TEST_F(TableTestFixture, ForEachKeyIgnoresEmptyLists) {
  const auto list1 = table->getUniqueOrCreate(key1);
  const auto list2 = table->getUniqueOrCreate(key2);
  const auto list3 = table->getUniqueOrCreate(key3);

  std::vector<std::string> keys;
  table->forEachKey(
      [&keys](const Bytes& key) { keys.push_back(key.toString()); });
  ASSERT_TRUE(keys.empty());
}

} // namespace internal
} // namespace multimap
