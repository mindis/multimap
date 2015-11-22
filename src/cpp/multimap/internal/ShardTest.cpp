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
#include "multimap/internal/Shard.hpp"

namespace multimap {
namespace internal {

TEST(ShardTest, IsNotDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<Shard>::value);
}

TEST(ShardTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<Shard>::value);
  ASSERT_FALSE(std::is_copy_assignable<Shard>::value);
}

TEST(ShardTest, IsNotMoveConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_move_constructible<Shard>::value);
  ASSERT_FALSE(std::is_move_assignable<Shard>::value);
}

struct ShardTestFixture : public testing::Test {
  void SetUp() override {
    directory = "/tmp/multimap.ShardTestFixture";
    boost::filesystem::remove_all(directory);
    boost::filesystem::create_directory(directory);
    Shard::Options options;
    options.create_if_missing = true;
    shard.reset(new Shard(directory / "shard", options));
    key1 = Bytes("k1");
    key2 = Bytes("k2");
    key3 = Bytes("k3");
  }

  void TearDown() override {
    shard.reset(); // Destructor flushes all data to disk.
    boost::filesystem::remove_all(directory);
  }

  boost::filesystem::path directory;
  std::unique_ptr<Shard> shard;
  Bytes key1;
  Bytes key2;
  Bytes key3;
};

TEST_F(ShardTestFixture, GetUniqueOrCreateInsertsNewKey) {
  shard->getUniqueOrCreate(key1);
  ASSERT_TRUE(shard->getShared(key1));
  ASSERT_TRUE(shard->getUnique(key1));

  shard->getUniqueOrCreate(key2);
  ASSERT_TRUE(shard->getShared(key2));
  ASSERT_TRUE(shard->getUnique(key2));
}

TEST_F(ShardTestFixture, InsertKeyThenGetSharedTwice) {
  shard->getUniqueOrCreate(key1);
  const auto list1 = shard->getShared(key1);
  ASSERT_TRUE(list1);
  const auto list2 = shard->getShared(key1);
  ASSERT_TRUE(list2);
}

TEST_F(ShardTestFixture, InsertKeyThenGetUniqueTwice) {
  shard->getUniqueOrCreate(key1);
  bool other_thread_got_unique_list = false;
  {
    const auto list = shard->getUnique(key1);
    ASSERT_TRUE(list);
    // We don't have `getUnique(key, wait_for_some_time)` yet,
    // so we start a new thread that tries to acquire the lock.
    std::thread thread([&] {
      shard->getUnique(key1);
      other_thread_got_unique_list = true;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_FALSE(other_thread_got_unique_list);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_TRUE(other_thread_got_unique_list);
}

TEST_F(ShardTestFixture, InsertKeyThenGetSharedAndGetUnique) {
  shard->getUniqueOrCreate(key1);
  bool other_thread_gots_unique_list = false;
  {
    const auto list = shard->getShared(key1);
    ASSERT_TRUE(list);
    // We don't have GetUnique(key, wait_for_some_time)
    // so we start a new thread that tries to acquire the lock.
    std::thread thread([&] {
      shard->getUnique(key1);
      other_thread_gots_unique_list = true;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_FALSE(other_thread_gots_unique_list);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_TRUE(other_thread_gots_unique_list);
}

TEST_F(ShardTestFixture, InsertKeyThenGetUniqueAndGetShared) {
  shard->getUniqueOrCreate(key1);
  bool other_thread_gots_shared_list = false;
  {
    const auto list = shard->getUnique(key1);
    ASSERT_TRUE(list);
    // We don't have GetShared(key, wait_for_some_time)
    // so we start a new thread that tries to acquire the lock.
    std::thread thread([&] {
      shard->getShared(key1);
      other_thread_gots_shared_list = true;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_FALSE(other_thread_gots_shared_list);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_TRUE(other_thread_gots_shared_list);
}

TEST_F(ShardTestFixture, InsertKeysThenGetAllShared) {
  shard->getUniqueOrCreate(key1);
  shard->getUniqueOrCreate(key2);
  bool other_thread_gots_shared_list = false;

  const auto list = shard->getShared(key1);
  ASSERT_TRUE(list);
  // We don't have GetShared(key, wait_for_some_time)
  // so we start a new thread that tries to acquire the lock.
  std::thread thread([&] {
    shard->getShared(key2);
    other_thread_gots_shared_list = true;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_TRUE(other_thread_gots_shared_list);
}

TEST_F(ShardTestFixture, InsertKeysThenGetAllUnique) {
  shard->getUniqueOrCreate(key1);
  shard->getUniqueOrCreate(key2);
  bool other_thread_gots_unique_list = false;

  const auto list = shard->getUnique(key1);
  ASSERT_TRUE(list);
  // We don't have GetUnique(key, wait_for_some_time)
  // so we start a new thread that tries to acquire the lock.
  std::thread thread([&] {
    shard->getUnique(key2);
    other_thread_gots_unique_list = true;
  });
  thread.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_TRUE(other_thread_gots_unique_list);
}

TEST_F(ShardTestFixture, InsertAndDeleteKeys) {
  shard->getUniqueOrCreate(key1);
  shard->getUniqueOrCreate(key2);

  ASSERT_TRUE(shard->getShared(key1));
  ASSERT_TRUE(shard->getUnique(key1));

  ASSERT_TRUE(shard->getShared(key2));
  ASSERT_TRUE(shard->getUnique(key2));

  ASSERT_FALSE(shard->getShared(key3));
  ASSERT_FALSE(shard->getUnique(key3));
}

TEST_F(ShardTestFixture, ForEachKeyIgnoresEmptyLists) {
  const auto list1 = shard->getUniqueOrCreate(key1);
  const auto list2 = shard->getUniqueOrCreate(key2);
  const auto list3 = shard->getUniqueOrCreate(key3);

  std::vector<std::string> keys;
  shard->forEachKey(
      [&keys](const Bytes& key) { keys.push_back(key.toString()); });
  ASSERT_TRUE(keys.empty());
}

} // namespace internal
} // namespace multimap
