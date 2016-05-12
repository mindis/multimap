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

#include <limits>
#include <string>
#include <thread>  // NOLINT
#include <type_traits>
#include <boost/filesystem/operations.hpp>  // NOLINT
#include "gmock/gmock.h"
#include "multimap/internal/Generator.h"
#include "multimap/internal/List.h"

namespace multimap {
namespace internal {

using testing::Eq;
using testing::ElementsAre;

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
// Appending and Iteration
// -----------------------------------------------------------------------------

class ListTestWithParam : public testing::TestWithParam<int> {
 public:
  void SetUp() override {
    directory_ = "/tmp/multimap.ListTestWithParam";
    boost::filesystem::remove_all(directory_);
    MT_ASSERT_TRUE(boost::filesystem::create_directory(directory_));
    store_ = Store(directory_ / "store", Options());
  }

  void TearDown() override {
    store_ = Store();  // Destructor flushes all data to disk.
    MT_ASSERT_TRUE(boost::filesystem::remove_all(directory_));
  }

  Store* getStore() { return &store_; }
  Arena* getArena() { return &arena_; }

 private:
  boost::filesystem::path directory_;
  Store store_;
  Arena arena_;
};

TEST_P(ListTestWithParam, AppendSmallValuesAndIterateOnce) {
  List list;
  List::Stats stats;
  for (int i = 0; i < GetParam(); i++) {
    ASSERT_TRUE(list.tryGetStats(&stats));
    ASSERT_EQ(0, stats.num_values_removed);
    ASSERT_EQ(i, stats.num_values_total);
    list.append(std::to_string(i), getStore(), getArena());
  }
  ASSERT_TRUE(list.tryGetStats(&stats));
  ASSERT_EQ(0, stats.num_values_removed);
  ASSERT_EQ(GetParam(), stats.num_values_total);

  auto iter = list.newIterator(*getStore());
  for (int i = 0; i < GetParam(); i++) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(GetParam() - i, iter->available());
    ASSERT_EQ(std::to_string(i), iter->next());
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(0, iter->available());
}

TEST_P(ListTestWithParam, AppendSmallValuesAndIterateTwice) {
  List list;
  List::Stats stats;
  for (int i = 0; i < GetParam(); i++) {
    ASSERT_TRUE(list.tryGetStats(&stats));
    ASSERT_EQ(0, stats.num_values_removed);
    ASSERT_EQ(i, stats.num_values_total);
    list.append(std::to_string(i), getStore(), getArena());
  }
  ASSERT_TRUE(list.tryGetStats(&stats));
  ASSERT_EQ(0, stats.num_values_removed);
  ASSERT_EQ(GetParam(), stats.num_values_total);

  auto iter = list.newIterator(*getStore());
  for (int i = 0; i < GetParam(); i++) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(GetParam() - i, iter->available());
    ASSERT_EQ(std::to_string(i), iter->next());
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(0, iter->available());

  iter = list.newIterator(*getStore());
  for (int i = 0; i < GetParam(); i++) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(GetParam() - i, iter->available());
    ASSERT_EQ(std::to_string(i), iter->next());
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(0, iter->available());
}

TEST_P(ListTestWithParam, AppendLargeValuesAndIterateOnce) {
  List list;
  List::Stats stats;
  SequenceGenerator generator;
  const auto value_size = getStore()->getBlockSize() * 2.3;
  for (int i = 0; i < GetParam(); i++) {
    ASSERT_TRUE(list.tryGetStats(&stats));
    ASSERT_EQ(0, stats.num_values_removed);
    ASSERT_EQ(i, stats.num_values_total);
    list.append(generator.nextof(value_size), getStore(), getArena());
  }
  ASSERT_TRUE(list.tryGetStats(&stats));
  ASSERT_EQ(0, stats.num_values_removed);
  ASSERT_EQ(GetParam(), stats.num_values_total);

  generator.reset();
  auto iter = list.newIterator(*getStore());
  for (int i = 0; i < GetParam(); i++) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(GetParam() - i, iter->available());
    ASSERT_EQ(generator.nextof(value_size), iter->next());
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(0, iter->available());
}

TEST_P(ListTestWithParam, AppendLargeValuesAndIterateTwice) {
  List list;
  List::Stats stats;
  SequenceGenerator generator;
  const auto value_size = getStore()->getBlockSize() * 2.3;
  for (int i = 0; i < GetParam(); i++) {
    ASSERT_TRUE(list.tryGetStats(&stats));
    ASSERT_EQ(0, stats.num_values_removed);
    ASSERT_EQ(i, stats.num_values_total);
    list.append(generator.nextof(value_size), getStore(), getArena());
  }
  ASSERT_TRUE(list.tryGetStats(&stats));
  ASSERT_EQ(0, stats.num_values_removed);
  ASSERT_EQ(GetParam(), stats.num_values_total);

  generator.reset();
  auto iter = list.newIterator(*getStore());
  for (int i = 0; i < GetParam(); i++) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(GetParam() - i, iter->available());
    ASSERT_EQ(generator.nextof(value_size), iter->next());
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(0, iter->available());

  generator.reset();
  iter = list.newIterator(*getStore());
  for (int i = 0; i < GetParam(); i++) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(GetParam() - i, iter->available());
    ASSERT_EQ(generator.nextof(value_size), iter->next());
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(0, iter->available());
}

TEST_P(ListTestWithParam, FlushWhileAppendingSmallValuesAndIterate) {
  List list;
  List::Stats stats;
  for (int i = 0; i < GetParam(); i++) {
    ASSERT_TRUE(list.tryGetStats(&stats));
    ASSERT_EQ(0, stats.num_values_removed);
    ASSERT_EQ(i, stats.num_values_total);
    if (stats.num_values_total % 5 == 0) {
      list.flushUnlocked(getStore());
    }
    list.append(std::to_string(i), getStore(), getArena());
  }

  auto iter = list.newIterator(*getStore());
  for (int i = 0; i < GetParam(); i++) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(GetParam() - i, iter->available());
    ASSERT_EQ(std::to_string(i), iter->next());
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(0, iter->available());
}

TEST_P(ListTestWithParam, FlushWhileAppendingLargeValuesAndIterate) {
  List list;
  List::Stats stats;
  SequenceGenerator generator;
  const auto value_size = getStore()->getBlockSize() * 2.3;
  for (int i = 0; i < GetParam(); i++) {
    ASSERT_TRUE(list.tryGetStats(&stats));
    ASSERT_EQ(0, stats.num_values_removed);
    ASSERT_EQ(i, stats.num_values_total);
    list.flushUnlocked(getStore());
    list.append(generator.nextof(value_size), getStore(), getArena());
  }

  generator.reset();
  auto iter = list.newIterator(*getStore());
  for (int i = 0; i < GetParam(); i++) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(GetParam() - i, iter->available());
    ASSERT_EQ(generator.nextof(value_size), iter->next());
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(0, iter->available());
}

TEST_P(ListTestWithParam, ForEachValueVisitsEachValue) {
  List list;
  for (int i = 0; i < GetParam(); i++) {
    list.append(std::to_string(i), getStore(), getArena());
  }

  int counter = 0;
  list.forEachValue([&counter](const Slice& value) {
    ASSERT_EQ(std::to_string(counter), value);
    counter++;
  }, *getStore());
  ASSERT_EQ(GetParam(), counter);
}

INSTANTIATE_TEST_CASE_P(Parameterized, ListTestWithParam,
                        testing::Values(0, 1, 2, 10, 100, 1000, 1000000));

// -----------------------------------------------------------------------------
// Mutation
// -----------------------------------------------------------------------------

class ListTestFixture : public testing::Test {
 public:
  void SetUp() override {
    directory_ = "/tmp/multimap.ListTestFixture";
    boost::filesystem::remove_all(directory_);
    MT_ASSERT_TRUE(boost::filesystem::create_directory(directory_));
    store_ = Store(directory_ / "store", Options());
  }

  void TearDown() override {
    store_ = Store();  // Destructor flushes all data to disk.
    MT_ASSERT_TRUE(boost::filesystem::remove_all(directory_));
  }

  Store* getStore() { return &store_; }
  Arena* getArena() { return &arena_; }

 protected:
  boost::filesystem::path directory_;
  Store store_;
  Arena arena_;
};

TEST_F(ListTestFixture, RemoveFirstMatchOnlyRemovesFirstMatch) {
  List list;
  const int factor = 10;
  const int num_values = 100;
  for (int i = 0; i < factor; i++) {
    for (int j = 0; j < num_values; j++) {
      list.append(std::to_string(j), getStore(), getArena());
    }
  }

  const std::string s23 = std::to_string(23);
  const auto is_23 = [&s23](const Slice& value) { return value == s23; };
  ASSERT_TRUE(list.removeFirstMatch(is_23, getStore()));

  bool is_first_match = true;
  auto iter = list.newIterator(*getStore());
  for (int i = 0; i < factor; i++) {
    for (int j = 0; j < num_values; j++) {
      const auto expected = std::to_string(j);
      if (is_23(expected) && is_first_match) {
        is_first_match = false;
      } else {
        ASSERT_TRUE(iter->hasNext());
        ASSERT_EQ(expected, iter->next());
      }
    }
  }
  ASSERT_FALSE(iter->hasNext());
}

TEST_F(ListTestFixture, RemoveAllMatchesRemovesAllMatches) {
  List list;
  const int factor = 10;
  const int num_values = 100;
  for (int i = 0; i < factor; i++) {
    for (int j = 0; j < num_values; j++) {
      list.append(std::to_string(j), getStore(), getArena());
    }
  }

  const std::string s23 = std::to_string(23);
  const auto is_23 = [&s23](const Slice& value) { return value == s23; };
  ASSERT_EQ(factor, list.removeAllMatches(is_23, getStore()));

  auto iter = list.newIterator(*getStore());
  for (int i = 0; i < factor; i++) {
    for (int j = 0; j < num_values; j++) {
      const auto expected = std::to_string(j);
      if (is_23(expected)) {
        // This value has been removed.
      } else {
        ASSERT_TRUE(iter->hasNext());
        ASSERT_EQ(expected, iter->next());
      }
    }
  }
  ASSERT_FALSE(iter->hasNext());
}

TEST_F(ListTestFixture, ReplaceFirstMatchOnlyReplacesFirstMatch) {
  List list;
  const int factor = 10;
  const int num_values = 100;
  for (int i = 0; i < factor; i++) {
    for (int j = 0; j < num_values; j++) {
      list.append(std::to_string(j), getStore(), getArena());
    }
  }

  const std::string s23 = std::to_string(23);
  const std::string s42 = std::to_string(42);
  const auto is_23 = [&s23](const Slice& value) { return value == s23; };
  const auto map_23_to_42 = [&s42, &is_23](const Slice& value) {
    return is_23(value) ? toBytes(s42) : Bytes();
  };
  ASSERT_TRUE(list.replaceFirstMatch(map_23_to_42, getStore(), getArena()));

  bool is_first_match = true;
  auto iter = list.newIterator(*getStore());
  for (int i = 0; i < factor; i++) {
    for (int j = 0; j < num_values; j++) {
      const auto expected = std::to_string(j);
      if (is_23(expected) && is_first_match) {
        is_first_match = false;
        // The "42" replacement is appended to the end of the list.
      } else {
        ASSERT_TRUE(iter->hasNext());
        ASSERT_EQ(expected, iter->next());
      }
    }
  }
  ASSERT_TRUE(iter->hasNext());
  ASSERT_EQ(s42, iter->next());
  ASSERT_FALSE(iter->hasNext());
}

TEST_F(ListTestFixture, ReplaceAllMatchesReplacesAllMatches) {
  List list;
  const int factor = 10;
  const int num_values = 100;
  for (int i = 0; i < factor; i++) {
    for (int j = 0; j < num_values; j++) {
      list.append(std::to_string(j), getStore(), getArena());
    }
  }

  const std::string s23 = std::to_string(23);
  const std::string s42 = std::to_string(42);
  const auto is_23 = [&s23](const Slice& value) { return value == s23; };
  const auto map_23_to_42 = [&s42, &is_23](const Slice& value) {
    return is_23(value) ? toBytes(s42) : Bytes();
  };
  ASSERT_EQ(factor,
            list.replaceAllMatches(map_23_to_42, getStore(), getArena()));

  auto iter = list.newIterator(*getStore());
  for (int i = 0; i < factor; i++) {
    for (int j = 0; j < num_values; j++) {
      const auto expected = std::to_string(j);
      if (is_23(expected)) {
        // The "42" replacement is appended to the end of the list.
      } else {
        ASSERT_TRUE(iter->hasNext());
        ASSERT_EQ(expected, iter->next());
      }
    }
  }
  for (int i = 0; i < factor; i++) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(s42, iter->next());
  }
  ASSERT_FALSE(iter->hasNext());
}

TEST_F(ListTestFixture, ClearRemovesAllValues) {
  List list;
  const int num_values = 1000;
  for (int i = 0; i < num_values; i++) {
    list.append(std::to_string(i), getStore(), getArena());
  }
  ASSERT_EQ(0, list.getStatsUnlocked().num_values_removed);
  ASSERT_EQ(num_values, list.getStatsUnlocked().num_values_total);
  ASSERT_EQ(num_values, list.getStatsUnlocked().num_values_valid());
  ASSERT_FALSE(list.empty());

  // Clear list.
  ASSERT_EQ(num_values, list.clear());
  ASSERT_EQ(num_values, list.getStatsUnlocked().num_values_removed);
  ASSERT_EQ(num_values, list.getStatsUnlocked().num_values_total);
  ASSERT_EQ(0, list.getStatsUnlocked().num_values_valid());
  ASSERT_TRUE(list.empty());

  // Append again.
  for (int i = 0; i < num_values; i++) {
    list.append(std::to_string(i), getStore(), getArena());
  }
  ASSERT_EQ(num_values, list.getStatsUnlocked().num_values_removed);
  ASSERT_EQ(num_values * 2, list.getStatsUnlocked().num_values_total);
  ASSERT_EQ(num_values, list.getStatsUnlocked().num_values_valid());
  ASSERT_FALSE(list.empty());

  // Clear list again.
  ASSERT_EQ(num_values, list.clear());
  ASSERT_EQ(num_values * 2, list.getStatsUnlocked().num_values_removed);
  ASSERT_EQ(num_values * 2, list.getStatsUnlocked().num_values_total);
  ASSERT_EQ(0, list.getStatsUnlocked().num_values_valid());
  ASSERT_TRUE(list.empty());

  // Append again.
  for (int i = 0; i < num_values; i++) {
    list.append(std::to_string(i), getStore(), getArena());
  }
  ASSERT_EQ(num_values * 2, list.getStatsUnlocked().num_values_removed);
  ASSERT_EQ(num_values * 3, list.getStatsUnlocked().num_values_total);
  ASSERT_EQ(num_values, list.getStatsUnlocked().num_values_valid());
  ASSERT_FALSE(list.empty());
}

// -----------------------------------------------------------------------------
// Serialization
// -----------------------------------------------------------------------------

class ListTestFixture2 : public ListTestFixture {
 public:
  void SetUp() override {
    ListTestFixture::SetUp();
    stream_ = mt::newFileStream(directory_ / "lists", mt::in_out_trunc);
  }

  void TearDown() override {
    stream_.reset();
    ListTestFixture::TearDown();
  }

  std::iostream* getStream() { return stream_.get(); }

 private:
  mt::InputOutputStream stream_;
};

TEST_F(ListTestFixture2, WriteListToFileThenReadBackAndIterate) {
  List list;
  const int num_values = 1000;
  for (int i = 0; i < num_values; i++) {
    list.append(std::to_string(i), getStore(), getArena());
  }
  list.flushUnlocked(getStore());

  list.writeToStream(getStream());
  getStream()->seekg(0);
  list = List::readFromStream(getStream());

  auto iter = list.newIterator(*getStore());
  for (int i = 0; i < num_values; i++) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(num_values - i, iter->available());
    ASSERT_EQ(std::to_string(i), iter->next());
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(0, iter->available());
}

// -----------------------------------------------------------------------------
// Concurrency
// -----------------------------------------------------------------------------

TEST_F(ListTestFixture, ReaderDoesNotBlockReader) {
  List list;

  // First reader
  auto iter = list.newIterator(*getStore());

  // Second reader
  bool second_reader_has_finished = false;
  std::thread([&] {  // NOLINT
    auto iter = list.newIterator(*getStore());
    second_reader_has_finished = true;
  }).join();

  ASSERT_TRUE(second_reader_has_finished);
}

TEST_F(ListTestFixture, ReaderDoesNotBlockReader2) {
  List list;

  // First reader
  auto iter = list.newIterator(*getStore());

  // Second readers
  List::Stats stats;
  ASSERT_TRUE(list.tryGetStats(&stats));
  ASSERT_TRUE(list.empty());
}

TEST_F(ListTestFixture, ReaderBlocksWriter) {
  List list;

  // Reader
  auto iter = list.newIterator(*getStore());

  // Writer
  bool writer_has_finished = false;
  std::thread writer([&] {  // NOLINT
    Arena arena;
    list.append("value", getStore(), getArena());
    writer_has_finished = true;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_FALSE(writer_has_finished);

  iter.reset();  // Releases reader lock

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_TRUE(writer_has_finished);
  writer.join();
}

TEST_F(ListTestFixture, WriterBlocksReader) {
  List list;
  list.append("value", getStore(), getArena());

  // Writer
  std::thread writer([&] {  // NOLINT
    list.removeFirstMatch([](const Slice& /* value */) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      return false;
    }, getStore());
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Reader
  bool reader_has_finished = false;
  std::thread reader([&] {  // NOLINT
    auto iter = list.newIterator(*getStore());
    reader_has_finished = true;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_FALSE(reader_has_finished);

  writer.join();  // Wait for release of writer lock

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_TRUE(reader_has_finished);
  reader.join();
}

TEST_F(ListTestFixture, WriterBlocksReader2) {
  List list;
  list.append("value", getStore(), getArena());

  // Writer
  std::thread writer([&] {  // NOLINT
    list.removeFirstMatch([](const Slice& /* value */) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      return false;
    }, getStore());
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Reader
  List::Stats stats;
  ASSERT_FALSE(list.tryGetStats(&stats));

  writer.join();  // Wait for release of writer lock

  ASSERT_TRUE(list.tryGetStats(&stats));
}

TEST_F(ListTestFixture, WriterBlocksWriter) {
  List list;
  list.append("value", getStore(), getArena());

  // First writer
  std::thread writer1([&] {  // NOLINT
    const auto removed = list.removeFirstMatch([](const Slice& /* value */) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      return false;
    }, getStore());
    ASSERT_FALSE(removed);
  });

  // Second writer
  bool second_writer_has_finished = false;
  std::thread writer2([&] {  // NOLINT
    const auto removed = list.removeFirstMatch(
        [](const Slice& /* value */) { return false; }, getStore());
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
// Varint encoding
// -----------------------------------------------------------------------------

struct VarintTestFixture : public testing::Test {
  void SetUp() override {
    std::memset(b5, 0, sizeof b5);
    std::memset(b25, 0, sizeof b25);
  }

  byte b5[5];
  byte b25[25];

  byte* b5end = std::end(b5);
  byte* b25end = std::end(b25);
};

TEST_F(VarintTestFixture, WriteMinVarintWithTrueFlag) {
  ASSERT_EQ(1, writeVarint32AndFlag(b5, b5end, 0, true));
  ASSERT_THAT(b5, ElementsAre(0x40, 0x00, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMinVarintWithFalseFlag) {
  ASSERT_EQ(1, writeVarint32AndFlag(b5, b5end, 0, false));
  ASSERT_THAT(b5, ElementsAre(0x00, 0x00, 0x00, 0x00, 0x00));
}

TEST_F(VarintTestFixture, WriteMaxVarintWithTrueFlag) {
  ASSERT_EQ(5, writeVarint32AndFlag(
                   b5, b5end, std::numeric_limits<uint32_t>::max(), true));
  ASSERT_THAT(b5, ElementsAre(0xFF, 0xFF, 0xFF, 0xFF, 0x1F));
}

TEST_F(VarintTestFixture, WriteMaxVarintWithFalseFlag) {
  ASSERT_EQ(5, writeVarint32AndFlag(
                   b5, b5end, std::numeric_limits<uint32_t>::max(), false));
  ASSERT_THAT(b5, ElementsAre(0xBF, 0xFF, 0xFF, 0xFF, 0x1F));
}

TEST_F(VarintTestFixture, ReadMinVarintWithTrueFlag) {
  writeVarint32AndFlag(b5, b5end, 0, true);
  bool flag = false;
  uint32_t value = -1;
  ASSERT_EQ(1, readVarint32AndFlag(b5, b5end, &value, &flag));
  ASSERT_EQ(0, value);
  ASSERT_TRUE(flag);
}

TEST_F(VarintTestFixture, ReadMinVarintWithFalseFlag) {
  writeVarint32AndFlag(b5, b5end, 0, false);
  bool flag = true;
  uint32_t value = -1;
  ASSERT_EQ(1, readVarint32AndFlag(b5, b5end, &value, &flag));
  ASSERT_EQ(0, value);
  ASSERT_FALSE(flag);
}

TEST_F(VarintTestFixture, ReadMaxVarintWithTrueFlag) {
  writeVarint32AndFlag(b5, b5end, std::numeric_limits<uint32_t>::max(), true);
  bool flag = false;
  uint32_t value = 0;
  ASSERT_EQ(5, readVarint32AndFlag(b5, b5end, &value, &flag));
  ASSERT_EQ(std::numeric_limits<uint32_t>::max(), value);
  ASSERT_TRUE(flag);
}

TEST_F(VarintTestFixture, ReadMaxVarintWithFalseFlag) {
  writeVarint32AndFlag(b5, b5end, std::numeric_limits<uint32_t>::max(), false);
  bool flag = true;
  uint32_t value = 0;
  ASSERT_EQ(5, readVarint32AndFlag(b5, b5end, &value, &flag));
  ASSERT_EQ(std::numeric_limits<uint32_t>::max(), value);
  ASSERT_FALSE(flag);
}

TEST_F(VarintTestFixture, WriteAndReadMultipleVarintsWithTrueFlags) {
  // clang-format off
  const uint32_t v1 = 23;
  const uint32_t v2 = 23 << 7;
  const uint32_t v3 = 23 << 14;
  const uint32_t v4 = 23 << 21;
  const uint32_t v5 = 23 << 28;

  byte* pos = b25;
  bool flag = true;
  ASSERT_EQ(1, writeVarint32AndFlag(pos, b25end, v1, flag)); pos += 1;
  ASSERT_EQ(2, writeVarint32AndFlag(pos, b25end, v2, flag)); pos += 2;
  ASSERT_EQ(3, writeVarint32AndFlag(pos, b25end, v3, flag)); pos += 3;
  ASSERT_EQ(4, writeVarint32AndFlag(pos, b25end, v4, flag)); pos += 4;
  ASSERT_EQ(5, writeVarint32AndFlag(pos, b25end, v5, flag)); pos += 5;

  pos = b25;
  flag = false;
  uint32_t value = 0;
  ASSERT_EQ(1, readVarint32AndFlag(pos, b25end, &value, &flag)); pos += 1;
  ASSERT_EQ(v1, value);
  ASSERT_TRUE(flag);

  flag = false;
  ASSERT_EQ(2, readVarint32AndFlag(pos, b25end, &value, &flag)); pos += 2;
  ASSERT_EQ(v2, value);
  ASSERT_TRUE(flag);

  flag = false;
  ASSERT_EQ(3, readVarint32AndFlag(pos, b25end, &value, &flag)); pos += 3;
  ASSERT_EQ(v3, value);
  ASSERT_TRUE(flag);

  flag = false;
  ASSERT_EQ(4, readVarint32AndFlag(pos, b25end, &value, &flag)); pos += 4;
  ASSERT_EQ(v4, value);
  ASSERT_TRUE(flag);

  flag = false;
  ASSERT_EQ(5, readVarint32AndFlag(pos, b25end, &value, &flag)); pos += 5;
  ASSERT_EQ(v5, value);
  ASSERT_TRUE(flag);
  // clang-format on
}

TEST_F(VarintTestFixture, WriteAndReadMultipleVarintsWithFalseFlags) {
  // clang-format off
  const uint32_t v1 = 23;
  const uint32_t v2 = 23 << 7;
  const uint32_t v3 = 23 << 14;
  const uint32_t v4 = 23 << 21;
  const uint32_t v5 = 23 << 28;

  byte* pos = b25;
  bool flag = false;
  ASSERT_EQ(1, writeVarint32AndFlag(pos, b25end, v1, flag)); pos += 1;
  ASSERT_EQ(2, writeVarint32AndFlag(pos, b25end, v2, flag)); pos += 2;
  ASSERT_EQ(3, writeVarint32AndFlag(pos, b25end, v3, flag)); pos += 3;
  ASSERT_EQ(4, writeVarint32AndFlag(pos, b25end, v4, flag)); pos += 4;
  ASSERT_EQ(5, writeVarint32AndFlag(pos, b25end, v5, flag)); pos += 5;

  pos = b25;
  flag = true;
  uint32_t value = 0;
  ASSERT_EQ(1, readVarint32AndFlag(pos, b25end, &value, &flag)); pos += 1;
  ASSERT_EQ(v1, value);
  ASSERT_FALSE(flag);

  flag = true;
  ASSERT_EQ(2, readVarint32AndFlag(pos, b25end, &value, &flag)); pos += 2;
  ASSERT_EQ(v2, value);
  ASSERT_FALSE(flag);

  flag = true;
  ASSERT_EQ(3, readVarint32AndFlag(pos, b25end, &value, &flag)); pos += 3;
  ASSERT_EQ(v3, value);
  ASSERT_FALSE(flag);

  flag = true;
  ASSERT_EQ(4, readVarint32AndFlag(pos, b25end, &value, &flag)); pos += 4;
  ASSERT_EQ(v4, value);
  ASSERT_FALSE(flag);

  flag = true;
  ASSERT_EQ(5, readVarint32AndFlag(pos, b25end, &value, &flag)); pos += 5;
  ASSERT_EQ(v5, value);
  ASSERT_FALSE(flag);
  // clang-format on
}

TEST_F(VarintTestFixture, WriteAndReadMultipleVarintsWithTrueAndFalseFlags) {
  // clang-format off
  const uint32_t v1 = 23;
  const uint32_t v2 = 23 << 7;
  const uint32_t v3 = 23 << 14;
  const uint32_t v4 = 23 << 21;
  const uint32_t v5 = 23 << 28;

  byte* pos = b25;
  ASSERT_EQ(1, writeVarint32AndFlag(pos, b25end, v1, true));  pos += 1;
  ASSERT_EQ(2, writeVarint32AndFlag(pos, b25end, v2, false)); pos += 2;
  ASSERT_EQ(3, writeVarint32AndFlag(pos, b25end, v3, true));  pos += 3;
  ASSERT_EQ(4, writeVarint32AndFlag(pos, b25end, v4, false)); pos += 4;
  ASSERT_EQ(5, writeVarint32AndFlag(pos, b25end, v5, true));  pos += 5;

  pos = b25;
  bool flag = false;
  uint32_t value = 0;
  ASSERT_EQ(1, readVarint32AndFlag(pos, b25end, &value, &flag)); pos += 1;
  ASSERT_EQ(v1, value);
  ASSERT_TRUE(flag);

  ASSERT_EQ(2, readVarint32AndFlag(pos, b25end, &value, &flag)); pos += 2;
  ASSERT_EQ(v2, value);
  ASSERT_FALSE(flag);

  ASSERT_EQ(3, readVarint32AndFlag(pos, b25end, &value, &flag)); pos += 3;
  ASSERT_EQ(v3, value);
  ASSERT_TRUE(flag);

  ASSERT_EQ(4, readVarint32AndFlag(pos, b25end, &value, &flag)); pos += 4;
  ASSERT_EQ(v4, value);
  ASSERT_FALSE(flag);

  ASSERT_EQ(5, readVarint32AndFlag(pos, b25end, &value, &flag)); pos += 5;
  ASSERT_EQ(v5, value);
  ASSERT_TRUE(flag);
  // clang-format on
}

TEST_F(VarintTestFixture, WriteTrueFlag) {
  std::memset(b5, 0xBF, sizeof b5);
  setFlag(b5, true);
  ASSERT_THAT(b5, ElementsAre(0xFF, 0xBF, 0xBF, 0xBF, 0xBF));
}

TEST_F(VarintTestFixture, WriteFalseFlag) {
  std::memset(b5, 0xFF, sizeof b5);
  setFlag(b5, false);
  ASSERT_THAT(b5, ElementsAre(0xBF, 0xFF, 0xFF, 0xFF, 0xFF));
}

}  // namespace internal
}  // namespace multimap
