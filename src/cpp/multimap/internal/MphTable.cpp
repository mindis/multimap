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
#include <limits>
#include <unordered_map>
#include <boost/filesystem/operations.hpp>
#include "multimap/internal/Arena.hpp"

namespace multimap {
namespace internal {

namespace {

class ListIter : public Iterator {
 public:
  ListIter(const byte* data, uint32_t num_values)
      : data_(data), num_values_(num_values) {}

  size_t available() const override { return num_values_; }

  bool hasNext() const override { return num_values_ != 0; }

  Slice next() override {
    MT_REQUIRE_TRUE(hasNext());
    const Slice value = peekNext();
    data_ = value.end();
    num_values_--;
    return value;
  }

  Slice peekNext() override {
    MT_REQUIRE_TRUE(hasNext());
    return Slice::readFromBuffer(data_).first;
  }

 private:
  const byte* data_ = nullptr;
  uint32_t num_values_ = 0;
};

typedef std::vector<Slice> List;
typedef std::unordered_map<Slice, List> Map;
typedef std::vector<uint32_t> Table;

std::string getNameOfListsFile(const std::string& prefix) {
  return prefix + ".lists";
}

std::string getNameOfMphFile(const std::string& prefix) {
  return prefix + ".mph";
}

std::string getNameOfStatsFile(const std::string& prefix) {
  return prefix + ".stats";
}

std::string getNameOfTableFile(const std::string& prefix) {
  return prefix + ".table";
}

std::pair<Map, Arena> readRecordsFromFile(
    const boost::filesystem::path& filename) {
  Map map;
  Bytes key;
  Bytes value;
  Arena arena;
  uint32_t key_size;
  const auto stream = mt::open(filename, "r");
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

Mph buildMphFromKeys(const Map& map, const MphTable::Options& options) {
  uint32_t key_size;
  auto iter = map.begin();
  std::vector<const byte*> keys(map.size());
  for (size_t i = 0; i != keys.size(); ++i, ++iter) {
    const byte* full_key_begin = iter->first.begin() - sizeof key_size;
    keys[i] = full_key_begin;
  }
  Mph::Options mph_options;
  mph_options.algorithm = CMPH_BDZ;
  mph_options.verbose = options.verbose;
  return Mph::build(keys.data(), keys.size(), mph_options);
}

Stats writeMap(const std::string& prefix, const Map& map, const Mph& mph,
               bool verbose) {
  Stats stats;
  stats.block_size = 8;
  stats.num_keys_total = map.size();
  stats.num_keys_valid = map.size();

  Table table(map.size());
  const Bytes zeros(stats.block_size, 0);
  const auto lists_filename = getNameOfListsFile(prefix);
  if (verbose) mt::log() << "Writing " << lists_filename << std::endl;
  const auto lists_stream = mt::open(lists_filename, "w");
  for (const auto& entry : map) {
    // TODO Since Linux supports holes in files we actually don't need padding.
    auto offset = mt::tell(lists_stream.get());
    if (const auto remainder = offset % stats.block_size) {
      const auto padding = stats.block_size - remainder;
      mt::write(lists_stream.get(), &zeros, padding);
      offset += padding;
    }

    const Slice& key = entry.first;
    const List& list = entry.second;
    table[mph(key)] = offset / stats.block_size;
    key.writeToStream(lists_stream.get());
    const uint32_t num_values = list.size();
    mt::write(lists_stream.get(), &num_values, sizeof num_values);
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
  }
  if (stats.num_keys_total) {
    stats.key_size_avg /= stats.num_keys_total;
    stats.list_size_avg /= stats.num_keys_total;
  }
  auto offset = mt::tell(lists_stream.get());
  // TODO Since Linux supports holes in files we actually don't need padding.
  if (const auto remainder = offset % stats.block_size) {
    const auto padding = stats.block_size - remainder;
    mt::write(lists_stream.get(), &zeros, padding);
    offset += padding;
  }
  stats.num_blocks = offset / stats.block_size;

  const auto mph_filename = getNameOfMphFile(prefix);
  if (verbose) mt::log() << "Writing " << mph_filename << std::endl;
  mph.dump(mph_filename);

  const auto table_filename = getNameOfTableFile(prefix);
  if (verbose) mt::log() << "Writing " << table_filename << std::endl;
  const auto table_stream = mt::open(table_filename, "w");
  mt::write(table_stream.get(), table.data(),
            table.size() * sizeof(Table::value_type));

  const auto stats_filename = getNameOfStatsFile(prefix);
  if (verbose) mt::log() << "Writing " << stats_filename << std::endl;
  stats.writeToFile(stats_filename);

  return stats;
}

mt::AutoUnmapMemory mapFileIntoMemory(const boost::filesystem::path& filename) {
  const auto fd = mt::open(filename, O_RDONLY);
  const auto filesize = boost::filesystem::file_size(filename);
  return mt::mmap(filesize, PROT_READ, MAP_SHARED, fd.get(), 0);
}

const byte* getListBegin(const mt::AutoUnmapMemory& lists, size_t block_id,
                         size_t block_size) {
  return lists.addr() + block_id * block_size;
}

uint32_t getTableEntry(const mt::AutoUnmapMemory& table, size_t index) {
  return *reinterpret_cast<const Table::value_type*>(
             table.addr() + index * sizeof(Table::value_type));
}

size_t getTableSize(const mt::AutoUnmapMemory& table) {
  return table.length() / sizeof(Table::value_type);
}

Table makeCopy(const mt::AutoUnmapMemory& table) {
  Table copy(getTableSize(table));
  std::memcpy(copy.data(), table.addr(), table.length());
  return copy;
}

}  // namespace

uint32_t MphTable::Limits::maxKeySize() {
  return std::numeric_limits<int32_t>::max();
}

uint32_t MphTable::Limits::maxValueSize() {
  return std::numeric_limits<int32_t>::max();
}

MphTable::MphTable(const std::string& prefix)
    : mph_(getNameOfMphFile(prefix)),
      table_(mapFileIntoMemory(getNameOfTableFile(prefix))),
      lists_(mapFileIntoMemory(getNameOfListsFile(prefix))),
      stats_(Stats::readFromFile(getNameOfStatsFile(prefix))) {}

std::unique_ptr<Iterator> MphTable::get(const Slice& key) const {
  const uint32_t hash = mph_(key);
  if (hash < getTableSize(table_)) {
    // `hash` may be greater equal than table size for unknown keys.
    const uint32_t block_id = getTableEntry(table_, hash);
    const byte* begin = getListBegin(lists_, block_id, stats_.block_size);
    const Slice actual_key = Slice::readFromBuffer(begin).first;
    if (actual_key == key) {
      uint32_t num_values;
      begin = actual_key.end();
      std::memcpy(&num_values, begin, sizeof num_values);
      begin += sizeof num_values;
      return std::unique_ptr<Iterator>(new ListIter(begin, num_values));
    }
  }
  return Iterator::newEmptyInstance();
}

void MphTable::forEachKey(Procedure process) const {
  Table table_copy = makeCopy(table_);
  std::sort(table_copy.begin(), table_copy.end());
  // TODO Advise access pattern for lists_.
  for (auto block_id : table_copy) {
    const byte* begin = getListBegin(lists_, block_id, stats_.block_size);
    const Slice key = Slice::readFromBuffer(begin).first;
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
  Table table_copy = makeCopy(table_);
  std::sort(table_copy.begin(), table_copy.end());
  // TODO Advise access pattern for lists_.
  for (auto block_id : table_copy) {
    const byte* begin = getListBegin(lists_, block_id, stats_.block_size);
    const Slice key = Slice::readFromBuffer(begin).first;
    begin = key.end();
    uint32_t num_values;
    std::memcpy(&num_values, begin, sizeof num_values);
    begin += sizeof num_values;
    ListIter iter(begin, num_values);
    process(key, &iter);
  }
}

Stats MphTable::build(const std::string& prefix,
                      const boost::filesystem::path& source,
                      const Options& options) {
  if (options.verbose) mt::log() << "Reading " << source.string() << std::endl;
  auto map_and_arena = readRecordsFromFile(source);
  Map& map = map_and_arena.first;
  if (options.compare) {
    for (auto& entry : map) {
      std::sort(entry.second.begin(), entry.second.end(), options.compare);
    }
  }
  const Mph mph = buildMphFromKeys(map, options);
  return writeMap(prefix, map, mph, options.verbose);
}

Stats MphTable::stats(const std::string& prefix) {
  return Stats::readFromFile(getNameOfStatsFile(prefix));
}

void MphTable::forEachEntry(const std::string& prefix,
                            BinaryProcedure process) {
  MphTable(prefix).forEachEntry(process);
}

}  // namespace internal
}  // namespace multimap
