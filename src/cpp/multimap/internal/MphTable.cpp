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

#include "multimap/internal/MphTable.hpp"

#include <algorithm>
#include <unordered_map>
#include <boost/filesystem/operations.hpp>
#include "multimap/thirdparty/mt/assert.hpp"
#include "multimap/thirdparty/mt/check.hpp"
#include "multimap/thirdparty/mt/memory.hpp"
#include "multimap/thirdparty/mt/varint.hpp"
#include "multimap/Arena.hpp"

namespace multimap {
namespace internal {

namespace bfs = boost::filesystem;

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
typedef std::unordered_map<Slice, List> Map;
typedef std::vector<uint32_t> Table;

bfs::path getPathOfRecordsFile(const bfs::path& prefix) {
  return prefix.string() + ".records";
}

bfs::path getPathOfListsFile(const bfs::path& prefix) {
  return prefix.string() + ".lists";
}

bfs::path getPathOfMphFile(const bfs::path& prefix) {
  return prefix.string() + ".mph";
}

bfs::path getPathOfStatsFile(const bfs::path& prefix) {
  return prefix.string() + ".stats";
}

bfs::path getPathOfTableFile(const bfs::path& prefix) {
  return prefix.string() + ".table";
}

std::pair<Map, Arena> readRecordsFromFile(const bfs::path& file_path) {
  Map map;
  Bytes key;
  Bytes value;
  Arena arena;
  uint32_t key_size;
  const mt::AutoCloseFile stream = mt::fopen(file_path, "r");
  while (readBytesFromStream(stream.get(), &key) &&
         readBytesFromStream(stream.get(), &value)) {
    auto iter = map.find(key);
    if (iter == map.end()) {
      key_size = key.size();
      byte* new_full_key = arena.allocate(sizeof key_size + key_size);
      std::memcpy(new_full_key, &key_size, sizeof key_size);
      byte* new_key_data = new_full_key + sizeof key_size;
      std::memcpy(new_key_data, key.data(), key_size);
      iter = map.emplace(Slice(new_key_data, key_size), List()).first;
      // New keys are stored in a way that they serve as input for CMPH.
    }
    byte* new_value_data = arena.allocate(value.size());
    std::memcpy(new_value_data, value.data(), value.size());
    iter->second.emplace_back(new_value_data, value.size());
  }
  return std::make_pair(std::move(map), std::move(arena));
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

Stats writeMap(const bfs::path& prefix, const Map& map, const Mph& mph,
               bool verbose) {
  const bfs::path mph_file_path = getPathOfMphFile(prefix);
  if (verbose) {
    mt::log() << "Writing " << mph_file_path.string() << std::endl;
  }
  mph.writeToFile(mph_file_path);

  Stats stats;
  stats.block_size = 8;
  stats.num_keys_total = map.size();
  stats.num_keys_valid = map.size();

  Table table(map.size());
  const Bytes zeros(stats.block_size, 0);
  const bfs::path lists_file_path = getPathOfListsFile(prefix);
  if (verbose) {
    mt::log() << "Writing " << lists_file_path.string() << std::endl;
  }
  const mt::AutoCloseFile lists_stream = mt::fopen(lists_file_path, "w");
  for (const auto& entry : map) {
    auto offset = mt::ftell(lists_stream.get());
    if (const auto remainder = offset % stats.block_size) {
      const auto padding = stats.block_size - remainder;
      mt::fwriteAll(lists_stream.get(), &zeros, padding);
      offset += padding;
    }
    const Slice& key = entry.first;
    const List& list = entry.second;
    table[mph(key)] = offset / stats.block_size;

    key.writeToStream(lists_stream.get());
    mt::writeVarint32ToStream(list.size(), lists_stream.get());
    for (const Slice& value : list) {
      value.writeToStream(lists_stream.get());
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
  auto offset = mt::ftell(lists_stream.get());
  if (const auto remainder = offset % stats.block_size) {
    const auto padding = stats.block_size - remainder;
    mt::fwriteAll(lists_stream.get(), &zeros, padding);
    offset += padding;
  }
  stats.num_blocks = offset / stats.block_size;

  const bfs::path table_file_path = getPathOfTableFile(prefix);
  if (verbose) {
    mt::log() << "Writing " << table_file_path.string() << std::endl;
  }
  const mt::AutoCloseFile table_stream = mt::fopen(table_file_path, "w");
  mt::fwriteAll(table_stream.get(), table.data(),
                table.size() * sizeof(Table::value_type));

  const bfs::path stats_file_path = getPathOfStatsFile(prefix);
  if (verbose) {
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

MphTable::Builder::Builder(const bfs::path& prefix, const Options& options)
    : stream_(mt::fopen(getPathOfRecordsFile(prefix), "w")),
      prefix_(prefix),
      options_(options) {}

MphTable::Builder::~Builder() {
  if (stream_) {
    stream_.reset();
    bfs::remove(getPathOfRecordsFile(prefix_));
  }
}

void MphTable::Builder::put(const Slice& key, const Slice& value) {
  MT_REQUIRE_LE(key.size(), Limits::maxKeySize());
  MT_REQUIRE_LE(value.size(), Limits::maxValueSize());
  key.writeToStream(stream_.get());
  value.writeToStream(stream_.get());
}

Stats MphTable::Builder::build() {
  MT_REQUIRE_TRUE(stream_);
  stream_.reset();
  const bfs::path records_file_path = getPathOfRecordsFile(prefix_);
  auto map_and_arena = readRecordsFromFile(records_file_path);
  MT_ASSERT_TRUE(bfs::remove(records_file_path));
  Map& map = map_and_arena.first;
  if (options_.compare) {
    for (auto& entry : map) {
      std::sort(entry.second.begin(), entry.second.end(), options_.compare);
    }
  }
  const Mph mph = buildMphFromKeys(map);
  return writeMap(prefix_, map, mph, options_.verbose);
}

MphTable::MphTable(const bfs::path& prefix)
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

Stats MphTable::stats(const bfs::path& prefix) {
  return Stats::readFromFile(getPathOfStatsFile(prefix));
}

void MphTable::forEachEntry(const bfs::path& prefix, BinaryProcedure process) {
  MphTable(prefix).forEachEntry(process);
}

}  // namespace internal
}  // namespace multimap
