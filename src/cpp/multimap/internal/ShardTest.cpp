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
#include <boost/filesystem/operations.hpp>
#include "gmock/gmock.h"
#include "multimap/internal/Shard.hpp"

namespace multimap {
namespace internal {

using testing::Eq;
using testing::UnorderedElementsAre;

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

    prefix = directory / "shard";
    shard.reset(new Shard(prefix.string()));
  }

  void TearDown() override {
    shard.reset();  // Destructor flushes all data to disk.
    boost::filesystem::remove_all(directory);
  }

  boost::filesystem::path directory;
  boost::filesystem::path prefix;
  std::unique_ptr<Shard> shard;
  std::string key1 = "k1";
  std::string key2 = "k2";
  std::string key3 = "k3";
  std::string value = "v";
};

TEST_F(ShardTestFixture, PutAddsValueToList) {
  ASSERT_THAT(shard->get(key1).available(), Eq(0));

  shard->put(key1, value);
  ASSERT_THAT(shard->get(key1).available(), Eq(1));
  ASSERT_THAT(shard->getMutable(key1).available(), Eq(1));

  shard->put(key1, value);
  ASSERT_THAT(shard->get(key1).available(), Eq(2));
  ASSERT_THAT(shard->getMutable(key1).available(), Eq(2));

  shard->put(key1, value);
  ASSERT_THAT(shard->get(key1).available(), Eq(3));
  ASSERT_THAT(shard->getMutable(key1).available(), Eq(3));
}

TEST_F(ShardTestFixture, GetTwiceForDifferentListsDoesNotBlock) {
  shard->put(key1, value);
  shard->put(key2, value);
  const auto iter1 = shard->get(key1);
  ASSERT_THAT(iter1.available(), Eq(1));
  const auto iter2 = shard->get(key2);
  ASSERT_THAT(iter2.available(), Eq(1));
}

TEST_F(ShardTestFixture, GetTwiceForSameListDoesNotBlock) {
  shard->put(key1, value);
  const auto iter1 = shard->get(key1);
  ASSERT_THAT(iter1.available(), Eq(1));
  const auto iter2 = shard->get(key1);
  ASSERT_THAT(iter2.available(), Eq(1));
}

TEST_F(ShardTestFixture, GetMutableTwiceForDifferentListsDoesNotBlock) {
  shard->put(key1, value);
  shard->put(key2, value);
  const auto iter1 = shard->getMutable(key1);
  ASSERT_THAT(iter1.available(), Eq(1));
  const auto iter2 = shard->getMutable(key2);
  ASSERT_THAT(iter2.available(), Eq(1));
}

TEST_F(ShardTestFixture, GetMutableTwiceForSameListBlocks) {
  shard->put(key1, value);
  bool other_thread_got_mutable_iter = false;
  {
    const auto iter = shard->getMutable(key1);
    ASSERT_THAT(iter.available(), Eq(1));
    // We don't have `getMutable(key, wait_for_some_time)` yet,
    // so we start a new thread that tries to acquire the lock.
    std::thread thread([&] {
      shard->getMutable(key1);
      other_thread_got_mutable_iter = true;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_FALSE(other_thread_got_mutable_iter);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_TRUE(other_thread_got_mutable_iter);
}

TEST_F(ShardTestFixture, GetAndThenGetMutableForSameListBlocks) {
  shard->put(key1, value);
  bool other_thread_got_mutable_iter = false;
  {
    const auto iter = shard->get(key1);
    ASSERT_THAT(iter.available(), Eq(1));
    // We don't have `get(key, wait_for_some_time)` yet,
    // so we start a new thread that tries to acquire the lock.
    std::thread thread([&] {
      shard->getMutable(key1);
      other_thread_got_mutable_iter = true;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_FALSE(other_thread_got_mutable_iter);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_TRUE(other_thread_got_mutable_iter);
}

TEST_F(ShardTestFixture, GetMutableAndThenGetForSameListBlocks) {
  shard->put(key1, value);
  bool other_thread_got_iter = false;
  {
    const auto iter = shard->getMutable(key1);
    ASSERT_THAT(iter.available(), Eq(1));
    // We don't have `getMutable(key, wait_for_some_time)` yet,
    // so we start a new thread that tries to acquire the lock.
    std::thread thread([&] {
      shard->get(key1);
      other_thread_got_iter = true;
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_FALSE(other_thread_got_iter);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_TRUE(other_thread_got_iter);
}

TEST_F(ShardTestFixture, ForEachKeyIgnoresEmptyLists) {
  shard->put(key1, value);
  shard->put(key2, value);
  shard->put(key3, value);

  shard->removeKey(key3);

  std::vector<std::string> keys;
  shard->forEachKey(
      [&keys](const Bytes& key) { keys.push_back(key.toString()); });
  ASSERT_THAT(keys, UnorderedElementsAre(key1, key2));
}

TEST_F(ShardTestFixture, PutValuesInTwoSessions) {
  shard->put(key1, value);
  shard.reset();  // Closes the shard before trying to reopen.
  shard.reset(new Shard(prefix, Shard::Options()));
  shard->put(key1, value);

  auto iter = shard->get(key1);
  for (std::size_t i = 0; i != 2; ++i) {
    ASSERT_TRUE(iter.hasNext());
    ASSERT_THAT(iter.next().toString(), Eq(value));
  }
}

// -----------------------------------------------------------------------------
// class Shard::Stats

TEST(ShardStatsTest, NamesAndToVectorHaveSameDimension) {
  const auto names = Shard::Stats::names();
  const auto vector = Shard::Stats().toVector();
  ASSERT_THAT(names.size(), Eq(vector.size()));
}

}  // namespace internal
}  // namespace multimap
