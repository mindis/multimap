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

#include "multimap/internal/MphTable.h"

#include <algorithm>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <boost/filesystem/operations.hpp>  // NOLINT
#include "multimap/thirdparty/mt/assert.h"
#include "multimap/thirdparty/mt/check.h"
#include "multimap/thirdparty/mt/memory.h"
#include "multimap/thirdparty/mt/varint.h"
#include "multimap/Arena.h"

namespace multimap {
namespace internal {

namespace fs = boost::filesystem;

namespace {

class ListIter : public Iterator {
 public:
  ListIter(const byte* buffer, size_t num_values)
      : pos_(buffer), num_values_(num_values) {}

  size_t available() const override { return num_values_; }

  bool hasNext() const override { return num_values_ != 0; }

  Slice next() override {
    const Slice value = peekNext();
    pos_ = value.end();
    num_values_--;
    return value;
  }

  Slice peekNext() override {
    MT_REQUIRE_TRUE(hasNext());
    return Slice::readFromBuffer(pos_);
  }

 private:
  const byte* pos_ = nullptr;
  size_t num_values_ = 0;
};

typedef std::vector<Slice> List;
typedef std::pair<List, Arena> ListAndArena;
typedef std::unordered_map<Slice, ListAndArena> Map;
typedef std::vector<uint32_t> Table;

fs::path getPathOfRecordsFile(const fs::path& prefix) {
  return prefix.string() + ".records";
}

fs::path getPathOfListsFile(const fs::path& prefix) {
  return prefix.string() + ".lists";
}

fs::path getPathOfMphFile(const fs::path& prefix) {
  return prefix.string() + ".mph";
}

fs::path getPathOfStatsFile(const fs::path& prefix) {
  return prefix.string() + ".stats";
}

fs::path getPathOfTableFile(const fs::path& prefix) {
  return prefix.string() + ".table";
}

// All keys in result.first are backed by the arena in result.second.
// Each value (list of slices) in result.first is backed by its own arena.
std::pair<Map, Arena> readRecordsFromFile(const fs::path& file_path) {
  Map map;
  Bytes key;
  Bytes value;
  Arena key_arena;
  uint32_t key_size;
  mt::InputStream istream = mt::newFileInputStream(file_path);
  while (readBytesFromStream(istream.get(), &key) &&
         readBytesFromStream(istream.get(), &value)) {
    auto iter = map.find(key);
    if (iter == map.end()) {
      key_size = key.size();
      byte* new_full_key = key_arena.allocate(sizeof key_size + key_size);
      std::memcpy(new_full_key, &key_size, sizeof key_size);
      byte* new_key_data = new_full_key + sizeof key_size;
      std::memcpy(new_key_data, key.data(), key_size);
      iter = map.emplace(Slice(new_key_data, key_size), ListAndArena()).first;
      // New keys are stored in a way that they work as input for CMPH.
    }
    byte* new_value_data = iter->second.second.allocate(value.size());
    std::memcpy(new_value_data, value.data(), value.size());
    iter->second.first.emplace_back(new_value_data, value.size());
  }
  return std::make_pair(std::move(map), std::move(key_arena));
}

Mph buildMphFromKeys(const Map& map) {
  uint32_t key_size;
  auto iter = map.begin();
  std::vector<const byte*> keys(map.size());
  for (size_t i = 0; i != keys.size(); ++i, ++iter) {
    const byte* full_key_begin = iter->first.begin() - sizeof key_size;
    keys[i] = full_key_begin;
  }
  return Mph::build(keys.data(), keys.size());
}

Stats writeMap(const fs::path& prefix, const Map& map, const Mph& mph,
               Options options) {
  const fs::path mph_file_path = getPathOfMphFile(prefix);
  if (options.verbose) {
    mt::log() << "Writing " << mph_file_path.string() << std::endl;
  }
  mph.writeToFile(mph_file_path);

  Table table(map.size());
  const Bytes zeros(options.block_size, 0);
  const fs::path lists_file_path = getPathOfListsFile(prefix);
  if (options.verbose) {
    mt::log() << "Writing " << lists_file_path.string() << std::endl;
  }
  mt::OutputStream lists_ostream = mt::newFileOutputStream(lists_file_path);

  Stats stats;
  stats.num_keys_total = map.size();
  stats.num_keys_valid = map.size();
  for (const auto& entry : map) {
    auto offset = lists_ostream->tellp();
    if (const auto remainder = offset % options.block_size) {
      const auto padding = options.block_size - remainder;
      mt::writeAll(lists_ostream.get(), &zeros, padding);
      offset += padding;
    }
    const Slice& key = entry.first;
    const List& list = entry.second.first;
    table[mph(key)] = offset / options.block_size;

    key.writeToStream(lists_ostream.get());
    mt::writeVarint32ToStream(list.size(), lists_ostream.get());
    for (const Slice& value : list) {
      value.writeToStream(lists_ostream.get());
    }

    stats.key_size_avg += key.size();
    stats.key_size_max = mt::max(stats.key_size_max, key.size());
    stats.key_size_min = stats.key_size_min
                             ? mt::min(stats.key_size_min, key.size())
                             : key.size();
    stats.list_size_avg += list.size();
    stats.list_size_max = mt::max(stats.list_size_max, list.size());
    stats.list_size_min = stats.list_size_min
                              ? mt::min(stats.list_size_min, list.size())
                              : list.size();
    stats.num_values_total += list.size();
    stats.num_values_valid += list.size();
  }
  if (stats.num_keys_total) {
    stats.key_size_avg /= stats.num_keys_total;
    stats.list_size_avg /= stats.num_keys_total;
  }
  auto offset = lists_ostream->tellp();
  if (const auto remainder = offset % options.block_size) {
    const auto padding = options.block_size - remainder;
    mt::writeAll(lists_ostream.get(), &zeros, padding);
    offset += padding;
  }
  stats.block_size = options.block_size;
  stats.num_blocks = offset / options.block_size;

  const fs::path table_file_path = getPathOfTableFile(prefix);
  if (options.verbose) {
    mt::log() << "Writing " << table_file_path.string() << std::endl;
  }
  mt::OutputStream table_ostream = mt::newFileOutputStream(table_file_path);
  mt::writeAll(table_ostream.get(), table.data(),
               table.size() * sizeof(Table::value_type));

  const fs::path stats_file_path = getPathOfStatsFile(prefix);
  if (options.verbose) {
    mt::log() << "Writing " << stats_file_path.string() << std::endl;
  }
  stats.writeToFile(stats_file_path);

  return stats;
}

const byte* getListBegin(const mt::AutoUnmapMemory& lists, size_t block_id,
                         size_t block_size) {
  return lists.data() + block_id * block_size;
}

uint32_t getTableEntry(const mt::AutoUnmapMemory& table, size_t index) {
  return *reinterpret_cast<const Table::value_type*>(
             table.data() + index * sizeof(Table::value_type));
}

size_t getTableSize(const mt::AutoUnmapMemory& table) {
  return table.size() / sizeof(Table::value_type);
}

Table makeCopy(const mt::AutoUnmapMemory& table) {
  Table copy(getTableSize(table));
  std::memcpy(copy.data(), table.data(), table.size());
  return copy;
}

}  // namespace

size_t MphTable::Limits::maxKeySize() {
  return std::numeric_limits<uint32_t>::max();
}

size_t MphTable::Limits::maxValueSize() {
  return std::numeric_limits<uint32_t>::max();
}

MphTable::Builder::Builder(const fs::path& prefix, const Options& options)
    : ostream_(mt::newFileOutputStream(getPathOfRecordsFile(prefix))),
      prefix_(prefix),
      options_(options) {}

MphTable::Builder::~Builder() {
  if (ostream_) {
    ostream_.reset();
    fs::remove(getPathOfRecordsFile(prefix_));
  }
}

void MphTable::Builder::put(const Slice& key, const Slice& value) {
  MT_REQUIRE_LE(key.size(), Limits::maxKeySize());
  MT_REQUIRE_LE(value.size(), Limits::maxValueSize());
  key.writeToStream(ostream_.get());
  value.writeToStream(ostream_.get());
}

Stats MphTable::Builder::build() {
  MT_REQUIRE_TRUE(ostream_);
  ostream_.reset();
  const fs::path records_file_path = getPathOfRecordsFile(prefix_);
  const size_t records_file_size = fs::file_size(records_file_path);
  const size_t max_block_id = std::numeric_limits<Table::value_type>::max();
  options_.block_size = (records_file_size / max_block_id) + 1;
  auto map_and_arena = readRecordsFromFile(records_file_path);
  MT_ASSERT_TRUE(fs::remove(records_file_path));

  Map& map = map_and_arena.first;
  if (options_.compare) {
    for (std::pair<const Slice, ListAndArena>& entry : map) {
      List& list = entry.second.first;
      std::sort(list.begin(), list.end(), options_.compare);
    }
  }

  if (options_.filter) {
    for (auto it = map.begin(); it != map.end();) {
      List new_list;
      Arena new_list_arena;
      const List& list = it->second.first;
      auto list_iter = makeRangeIterator(list.begin(), list.end());
      options_.filter(it->first, &list_iter, [&new_list, &new_list_arena](
                                                 const Slice& new_value) {
        if (!new_value.empty()) {
          new_list.push_back(new_value.makeCopy(&new_list_arena));
        }
      });
      if (new_list.empty()) {
        it = map.erase(it);
      } else {
        it->second = ListAndArena(new_list, std::move(new_list_arena));
      }
    }
  }

  const Mph mph = buildMphFromKeys(map);
  return writeMap(prefix_, map, mph, options_);
}

MphTable::MphTable(const fs::path& prefix)
    : mph_(Mph::readFromFile(getPathOfMphFile(prefix))),
      table_(mt::mmapFile(getPathOfTableFile(prefix), PROT_READ)),
      lists_(mt::mmapFile(getPathOfListsFile(prefix), PROT_READ)),
      stats_(Stats::readFromFile(getPathOfStatsFile(prefix))) {}

std::unique_ptr<Iterator> MphTable::get(const Slice& key) const {
  const uint32_t hash = mph_(key);
  MT_ASSERT_LT(hash, getTableSize(table_));  // Do we need this?
  const uint32_t block_id = getTableEntry(table_, hash);
  const byte* pos = getListBegin(lists_, block_id, stats_.block_size);
  const Slice actual_key = Slice::readFromBuffer(pos);
  if (key == actual_key) {
    uint32_t num_values;
    pos = actual_key.end();
    pos += mt::readVarint32FromBuffer(pos, &num_values);
    return std::unique_ptr<Iterator>(new ListIter(pos, num_values));
  }
  return Iterator::newEmptyInstance();
}

void MphTable::forEachKey(Procedure process) const {
  Table table_copy = makeCopy(table_);
  std::sort(table_copy.begin(), table_copy.end());
  for (uint32_t block_id : table_copy) {
    const byte* pos = getListBegin(lists_, block_id, stats_.block_size);
    const Slice key = Slice::readFromBuffer(pos);
    process(key);
  }
}

void MphTable::forEachValue(const Slice& key, Procedure process) const {
  const auto iter = get(key);
  while (iter->hasNext()) {
    process(iter->next());
  }
}

void MphTable::forEachEntry(BinaryProcedure process) const {
  uint32_t num_values;
  Table table_copy = makeCopy(table_);
  std::sort(table_copy.begin(), table_copy.end());
  for (auto block_id : table_copy) {
    const byte* pos = getListBegin(lists_, block_id, stats_.block_size);
    const Slice key = Slice::readFromBuffer(pos);
    pos = key.end();
    pos += mt::readVarint32FromBuffer(pos, &num_values);
    ListIter iter(pos, num_values);
    process(key, &iter);
  }
}

Stats MphTable::stats(const fs::path& prefix) {
  return Stats::readFromFile(getPathOfStatsFile(prefix));
}

void MphTable::forEachEntry(const fs::path& prefix, BinaryProcedure process) {
  MphTable(prefix).forEachEntry(process);
}

}  // namespace internal
}  // namespace multimap
