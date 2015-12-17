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

std::unique_ptr<Shard> openOrCreateShard(
    const boost::filesystem::path& prefix) {
  Shard::Options options;
  options.create_if_missing = true;
  return std::unique_ptr<Shard>(new Shard(prefix, options));
}

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
    boost::filesystem::remove_all(directory);
    boost::filesystem::create_directory(directory);
  }

  void TearDown() override { boost::filesystem::remove_all(directory); }

  const boost::filesystem::path directory = "/tmp/multimap.ShardTestFixture";
  const boost::filesystem::path prefix = directory / "shard";
  const std::string k1 = "k1";
  const std::string k2 = "k2";
  const std::string k3 = "k3";
  const std::string v1 = "v1";
  const std::string v2 = "v2";
  const std::string v3 = "v3";
};

TEST_F(ShardTestFixture, ConstructorThrowsIfNotExist) {
  ASSERT_THROW(Shard(this->prefix), std::runtime_error);
  // GCC complains when not using 'this' pointer.
}

TEST_F(ShardTestFixture, ConstructorWithDefaultOptionsThrowsIfNotExist) {
  Shard::Options options;
  ASSERT_THROW(Shard(prefix, options), std::runtime_error);
}

TEST_F(ShardTestFixture, ConstructorWithCreateIfMissingDoesNotThrow) {
  Shard::Options options;
  options.create_if_missing = true;
  ASSERT_NO_THROW(Shard(prefix, options));
}

TEST_F(ShardTestFixture, PutAppendsValuesToList) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  shard->put(k1, v2);
  shard->put(k1, v3);
  {
    auto iter = shard->get(k1);
    ASSERT_THAT(iter.next().toString(), Eq(v1));
    ASSERT_THAT(iter.next().toString(), Eq(v2));
    ASSERT_THAT(iter.next().toString(), Eq(v3));
    ASSERT_FALSE(iter.hasNext());
  }
  {
    auto iter = shard->getMutable(k1);
    ASSERT_THAT(iter.next().toString(), Eq(v1));
    ASSERT_THAT(iter.next().toString(), Eq(v2));
    ASSERT_THAT(iter.next().toString(), Eq(v3));
    ASSERT_FALSE(iter.hasNext());
  }
}

TEST_F(ShardTestFixture, GetReturnsEmptyListForNonExistingKey) {
  auto shard = openOrCreateShard(prefix);
  ASSERT_FALSE(shard->get(k1).hasNext());
  ASSERT_FALSE(shard->getMutable(k1).hasNext());
}

TEST_F(ShardTestFixture, GetTwiceForDifferentListsDoesNotBlock) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  shard->put(k2, v1);
  auto iter1 = shard->get(k1);
  ASSERT_TRUE(iter1.hasNext());
  auto iter2 = shard->get(k2);
  ASSERT_TRUE(iter2.hasNext());
}

TEST_F(ShardTestFixture, GetTwiceForSameListDoesNotBlock) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  auto iter1 = shard->get(k1);
  ASSERT_TRUE(iter1.hasNext());
  auto iter2 = shard->get(k1);
  ASSERT_TRUE(iter2.hasNext());
}

TEST_F(ShardTestFixture, GetMutableTwiceForDifferentListsDoesNotBlock) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  shard->put(k2, v1);
  auto iter1 = shard->getMutable(k1);
  ASSERT_TRUE(iter1.hasNext());
  auto iter2 = shard->getMutable(k2);
  ASSERT_TRUE(iter2.hasNext());
}

TEST_F(ShardTestFixture, GetMutableTwiceForSameListBlocks) {
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  bool other_thread_got_mutable_iter = false;
  {
    auto iter = shard->getMutable(k1);
    ASSERT_TRUE(iter.hasNext());
    // We don't have `getMutable(key, wait_for_some_time)` yet,
    // so we start a new thread that tries to acquire the lock.
    std::thread thread([&] {
      shard->getMutable(k1);
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
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  bool other_thread_got_mutable_iter = false;
  {
    auto iter = shard->get(k1);
    ASSERT_TRUE(iter.hasNext());
    // We don't have `get(key, wait_for_some_time)` yet,
    // so we start a new thread that tries to acquire the lock.
    std::thread thread([&] {
      shard->getMutable(k1);
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
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  bool other_thread_got_iter = false;
  {
    auto iter = shard->getMutable(k1);
    ASSERT_TRUE(iter.hasNext());
    // We don't have `getMutable(key, wait_for_some_time)` yet,
    // so we start a new thread that tries to acquire the lock.
    std::thread thread([&] {
      shard->get(k1);
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
  auto shard = openOrCreateShard(prefix);
  shard->put(k1, v1);
  shard->put(k2, v1);
  shard->put(k3, v1);

  shard->removeKey(k3);

  std::vector<std::string> keys;
  shard->forEachKey(
      [&keys](const Bytes& key) { keys.push_back(key.toString()); });
  ASSERT_THAT(keys, UnorderedElementsAre(k1, k2));
}

TEST_F(ShardTestFixture, PutValuesWithReopen) {
  {
    auto shard = openOrCreateShard(prefix);
    shard->put(k1, v1);
    shard->put(k2, v1);
    shard->put(k3, v1);
  }
  {
    auto shard = openOrCreateShard(prefix);
    shard->put(k1, v2);
    shard->put(k2, v2);
    shard->put(k3, v2);
  }
  {
    auto shard = openOrCreateShard(prefix);
    shard->put(k1, v3);
    shard->put(k2, v3);
    shard->put(k3, v3);
  }
  auto shard = openOrCreateShard(prefix);
  auto iter1 = shard->get(k1);
  ASSERT_THAT(iter1.next().toString(), Eq(v1));
  ASSERT_THAT(iter1.next().toString(), Eq(v2));
  ASSERT_THAT(iter1.next().toString(), Eq(v3));
  ASSERT_FALSE(iter1.hasNext());

  auto iter2 = shard->get(k2);
  ASSERT_THAT(iter2.next().toString(), Eq(v1));
  ASSERT_THAT(iter2.next().toString(), Eq(v2));
  ASSERT_THAT(iter2.next().toString(), Eq(v3));
  ASSERT_FALSE(iter2.hasNext());

  auto iter3 = shard->get(k3);
  ASSERT_THAT(iter3.next().toString(), Eq(v1));
  ASSERT_THAT(iter3.next().toString(), Eq(v2));
  ASSERT_THAT(iter3.next().toString(), Eq(v3));
  ASSERT_FALSE(iter3.hasNext());
}

// -----------------------------------------------------------------------------
// class Shard::Stats
// -----------------------------------------------------------------------------

TEST(ShardStatsTest, NamesAndToVectorHaveSameDimension) {
  const auto names = Shard::Stats::names();
  const auto vector = Shard::Stats().toVector();
  ASSERT_THAT(names.size(), Eq(vector.size()));
}

}  // namespace internal
}  // namespace multimap
