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
  List list;
  List::Stats stats;
  ASSERT_TRUE(list.tryGetStats(&stats));
  ASSERT_EQ(stats.num_values_removed, 0);
  ASSERT_EQ(stats.num_values_total, 0);
  ASSERT_EQ(list.empty(), true);
}

// -----------------------------------------------------------------------------
// class List / Concurrency
// -----------------------------------------------------------------------------

TEST(ListTest, ReaderDoesNotBlockReader) {
  List list;
  Store store;

  // First reader
  auto iter = list.newIterator(store);

  // Second reader
  bool second_reader_has_finished = false;
  std::thread([&] {
    auto iter = list.newIterator(store);
    second_reader_has_finished = true;
  }).join();

  ASSERT_TRUE(second_reader_has_finished);
}

TEST(ListTest, ReaderDoesNotBlockReader2) {
  List list;
  Store store;

  // First reader
  auto iter = list.newIterator(store);

  // Second readers
  List::Stats stats;
  ASSERT_TRUE(list.tryGetStats(&stats));
  ASSERT_TRUE(list.empty());
}

TEST(ListTest, ReaderBlocksWriter) {
  List list;
  Store store;

  // Reader
  auto iter = list.newIterator(store);

  // Writer
  bool writer_has_finished = false;
  std::thread writer([&] {
    Arena arena;
    list.append("value", &store, &arena);
    writer_has_finished = true;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_FALSE(writer_has_finished);

  iter.reset();  // Releases reader lock

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_TRUE(writer_has_finished);
  writer.join();
}

TEST(ListTest, WriterBlocksReader) {
  List list;
  Arena arena;
  Store store;
  list.append("value", &store, &arena);

  // Writer
  std::thread writer([&] {
    list.removeFirstMatch([](const Slice & /* value */) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      return false;
    }, &store);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Reader
  bool reader_has_finished = false;
  std::thread reader([&] {
    auto iter = list.newIterator(store);
    reader_has_finished = true;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_FALSE(reader_has_finished);

  writer.join();  // Wait for release of writer lock

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_TRUE(reader_has_finished);
  reader.join();
}

TEST(ListTest, WriterBlocksReader2) {
  List list;
  Arena arena;
  Store store;
  list.append("value", &store, &arena);

  // Writer
  std::thread writer([&] {
    list.removeFirstMatch([](const Slice & /* value */) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      return false;
    }, &store);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Reader
  List::Stats stats;
  ASSERT_FALSE(list.tryGetStats(&stats));

  writer.join();  // Wait for release of writer lock

  ASSERT_TRUE(list.tryGetStats(&stats));
}

TEST(ListTest, WriterBlocksWriter) {
  List list;
  Arena arena;
  Store store;
  list.append("value", &store, &arena);

  // First writer
  std::thread writer1([&] {
    const auto removed = list.removeFirstMatch([](const Slice & /* value */) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      return false;
    }, &store);
    ASSERT_FALSE(removed);
  });

  // Second writer
  bool second_writer_has_finished = false;
  std::thread writer2([&] {
    const auto removed = list.removeFirstMatch(
        [](const Slice & /* value */) { return false; }, &store);
    ASSERT_FALSE(removed);
    second_writer_has_finished = true;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_FALSE(second_writer_has_finished);

  writer1.join();  // Wait for release of first writer lock

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_TRUE(second_writer_has_finished);
  writer2.join();
}

// -----------------------------------------------------------------------------
// class List / Appending & Iteration
// -----------------------------------------------------------------------------

struct ListTestWithParam : testing::TestWithParam<uint32_t> {
  void SetUp() override {
    directory = "/tmp/multimap.ListTestIteration";
    boost::filesystem::remove_all(directory);
    MT_ASSERT_TRUE(boost::filesystem::create_directory(directory));

    store.reset(new Store(directory / "store", Options()));
  }

  void TearDown() override {
    store.reset();  // Destructor flushes all data to disk.
    MT_ASSERT_TRUE(boost::filesystem::remove_all(directory));
  }

  Store *getStore() { return store.get(); }
  Arena *getArena() { return &arena; }

 private:
  boost::filesystem::path directory;
  std::unique_ptr<Store> store;
  Arena arena;
};

TEST_P(ListTestWithParam, AppendSmallValuesAndIterateOnce) {
  List list;
  List::Stats stats;
  for (size_t i = 0; i != GetParam(); i++) {
    list.append(std::to_string(i), getStore(), getArena());
    ASSERT_TRUE(list.tryGetStats(&stats));
    ASSERT_EQ(stats.num_values_removed, 0);
    ASSERT_EQ(stats.num_values_total, i + 1);
  }
  ASSERT_TRUE(list.tryGetStats(&stats));
  ASSERT_EQ(stats.num_values_valid(), GetParam());

  auto iter = list.newIterator(*getStore());
  for (size_t i = 0; i != GetParam(); i++) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(iter->available(), GetParam() - i);
    ASSERT_EQ(iter->next(), std::to_string(i));
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(iter->available(), 0);
}

TEST_P(ListTestWithParam, AppendSmallValuesAndIterateTwice) {
  List list;
  List::Stats stats;
  for (size_t i = 0; i != GetParam(); i++) {
    list.append(std::to_string(i), getStore(), getArena());
    ASSERT_TRUE(list.tryGetStats(&stats));
    ASSERT_EQ(stats.num_values_removed, 0);
    ASSERT_EQ(stats.num_values_total, i + 1);
  }
  ASSERT_TRUE(list.tryGetStats(&stats));
  ASSERT_EQ(stats.num_values_valid(), GetParam());

  auto iter = list.newIterator(*getStore());
  for (size_t i = 0; i != GetParam(); i++) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(iter->available(), GetParam() - i);
    ASSERT_EQ(iter->next(), std::to_string(i));
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(iter->available(), 0);

  iter = list.newIterator(*getStore());
  for (size_t i = 0; i != GetParam(); i++) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(iter->available(), GetParam() - i);
    ASSERT_EQ(iter->next(), std::to_string(i));
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(iter->available(), 0);
}

TEST_P(ListTestWithParam, AppendLargeValuesAndIterateOnce) {
  List list;
  List::Stats stats;
  SequenceGenerator generator;
  const size_t size = getStore()->getBlockSize() * 2.5;
  for (size_t i = 0; i != GetParam(); i++) {
    list.append(generator.nextof(size), getStore(), getArena());
    ASSERT_TRUE(list.tryGetStats(&stats));
    ASSERT_EQ(stats.num_values_removed, 0);
    ASSERT_EQ(stats.num_values_total, i + 1);
  }
  ASSERT_TRUE(list.tryGetStats(&stats));
  ASSERT_EQ(stats.num_values_valid(), GetParam());

  generator.reset();
  auto iter = list.newIterator(*getStore());
  for (size_t i = 0; i != GetParam(); i++) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(iter->available(), GetParam() - i);
    ASSERT_EQ(iter->next(), generator.nextof(size));
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(iter->available(), 0);
}

TEST_P(ListTestWithParam, AppendLargeValuesAndIterateTwice) {
  List list;
  List::Stats stats;
  SequenceGenerator generator;
  const size_t size = getStore()->getBlockSize() * 2.5;
  for (size_t i = 0; i != GetParam(); i++) {
    list.append(generator.nextof(size), getStore(), getArena());
    ASSERT_TRUE(list.tryGetStats(&stats));
    ASSERT_EQ(stats.num_values_removed, 0);
    ASSERT_EQ(stats.num_values_total, i + 1);
  }
  ASSERT_TRUE(list.tryGetStats(&stats));
  ASSERT_EQ(stats.num_values_valid(), GetParam());

  generator.reset();
  auto iter = list.newIterator(*getStore());
  for (size_t i = 0; i != GetParam(); i++) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(iter->available(), GetParam() - i);
    ASSERT_EQ(iter->next(), generator.nextof(size));
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(iter->available(), 0);

  generator.reset();
  iter = list.newIterator(*getStore());
  for (size_t i = 0; i != GetParam(); i++) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(iter->available(), GetParam() - i);
    ASSERT_EQ(iter->next(), generator.nextof(size));
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(iter->available(), 0);
}

TEST_P(ListTestWithParam, FlushValuesBetweenAppendingAndIterate) {
  List list;
  List::Stats stats;
  const auto part_size = 1 + GetParam() / 5;
  for (size_t i = 0; i != GetParam(); i++) {
    list.append(std::to_string(i), getStore(), getArena());
    ASSERT_TRUE(list.tryGetStats(&stats));
    if (stats.num_values_valid() % part_size == 0) {
      ASSERT_TRUE(list.tryFlush(getStore()));
    }
  }

  auto iter = list.newIterator(*getStore());
  for (size_t i = 0; i != GetParam(); i++) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_THAT(iter->next(), Eq(std::to_string(i)));
  }
}

TEST_P(ListTestWithParam, RemoveEvery23thValue) {
  List list;
  for (size_t i = 0; i != GetParam(); i++) {
    list.append(std::to_string(i), getStore(), getArena());
  }

  const auto num_removed = list.removeAllMatches([](const Slice &value) {
    return std::stoi(value.toString()) % 23 == 0;
  }, getStore());

  auto iter = list.newIterator(*getStore());
  ASSERT_EQ(iter->available(), GetParam() - num_removed);
  for (size_t i = 0; iter->hasNext(); i++) {
    if (i % 23 != 0) {
      ASSERT_NE(std::stoi(iter->next().toString()) % 23, 0);
    }
  }
  ASSERT_EQ(iter->available(), 0);
  ASSERT_FALSE(iter->hasNext());
}

INSTANTIATE_TEST_CASE_P(Parameterized, ListTestWithParam,
                        testing::Values(0, 1, 2, 10, 100, 1000, 1000000));

}  // namespace internal
}  // namespace multimap
