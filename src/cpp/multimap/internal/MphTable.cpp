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

  Range next() override {
    MT_REQUIRE_TRUE(hasNext());
    const Range value = peekNext();
    data_ = value.end();
    num_values_--;
    return value;
  }

  Range peekNext() override {
    MT_REQUIRE_TRUE(hasNext());
    return Range::readFromBuffer(data_);
  }

 private:
  const byte* data_ = nullptr;
  uint32_t num_values_ = 0;
};

typedef std::vector<Range> List;
typedef std::unordered_map<Range, List> Map;
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

std::pair<Map, Arena> readRecordsFromFile(const std::string& filename) {
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
      iter = map.emplace(Range(new_key_data, key_size), List()).first;
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
  mph_options.quiet = options.quiet;
  return Mph::build(keys.data(), keys.size(), mph_options);
}

Table writeListsToFile(const std::string& filename, const Map& map,
                       const Mph& mph) {
  Table table(map.size());
  const auto alignment = 8;
  const Bytes zeros(alignment, 0);
  const auto stream = mt::open(filename, "w");
  for (const auto& entry : map) {
    auto offset = mt::tell(stream.get());
    if (const auto remainder = offset % alignment) {
      const auto padding = alignment - remainder;
      mt::write(stream.get(), &zeros, padding);
      offset += padding;
    }
    table[mph(entry.first)] = offset / alignment;
    entry.first.writeToStream(stream.get());
    const uint32_t num_values = entry.second.size();
    mt::write(stream.get(), &num_values, sizeof num_values);
    for (const auto& value : entry.second) {
      value.writeToStream(stream.get());
    }
  }
  return table;
}

Table readTableFromFile(const std::string& filename) {
  Table table;
  Table::value_type address;
  const auto stream = mt::open(filename, "r");
  while (mt::tryRead(stream.get(), &address, sizeof address)) {
    table.push_back(address);
  }
  table.shrink_to_fit();
  return table;
}

void writeTableToFile(const std::string& filename, const Table& table) {
  const auto stream = mt::open(filename, "w");
  mt::write(stream.get(), table.data(),
             table.size() * sizeof(Table::value_type));
}

Range mapListsIntoMemory(const std::string& filename) {
  const auto fd = mt::open(filename, O_RDONLY);
  const auto filesize = boost::filesystem::file_size(filename);
  const auto data = mt::mmap(filesize, PROT_READ, MAP_SHARED, fd.get(), 0);
  return Range(data, filesize);
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
      table_(readTableFromFile(getNameOfTableFile(prefix))),
      lists_(mapListsIntoMemory(getNameOfListsFile(prefix))),
      stats_(Stats::readFromFile(getNameOfStatsFile(prefix))) {
}

std::unique_ptr<Iterator> MphTable::get(const Range& key) const {
  const uint32_t hash = mph_(key);
  if (hash < table_.size()) {
    const byte* begin = lists_.begin() + table_[hash] * stats_.block_size;
    const Range actual_key = Range::readFromBuffer(begin);
    if (actual_key == key) {
      uint32_t num_values;
      begin = actual_key.end();
      std::memcpy(&num_values, begin, sizeof num_values);
      begin += sizeof num_values;
      return std::unique_ptr<Iterator>(new ListIter(begin, num_values));
    }
  }
  return Iterator::newEmptyInstance();
  // `hash` may be greater equal than `table_.size()` for unknown keys.
}

MphTable::Stats MphTable::build(const std::string& prefix,
                                const std::string& input,
                                const Options& options) {
  if (!options.quiet) mt::log() << "Reading " << input << std::endl;
  auto map_and_arena = readRecordsFromFile(input);
  auto& map = map_and_arena.first;
  if (options.compare) {
    for (auto& entry : map) {
      std::sort(entry.second.begin(), entry.second.end(), options.compare);
    }
  }
  const auto mph = buildMphFromKeys(map, options);
  const auto mph_filename = getNameOfMphFile(prefix);
  if (!options.quiet) mt::log() << "Writing " << mph_filename << std::endl;
  mph.dump(mph_filename);

  const auto data_filename = getNameOfListsFile(prefix);
  if (!options.quiet) mt::log() << "Writing " << data_filename << std::endl;
  const auto table = writeListsToFile(data_filename, map, mph);

  const auto table_filename = getNameOfTableFile(prefix);
  if (!options.quiet) mt::log() << "Writing " << table_filename << std::endl;
  writeTableToFile(table_filename, table);

  Stats stats;  // TODO
  const auto stats_filename = getNameOfStatsFile(prefix);
  if (!options.quiet) mt::log() << "Writing " << stats_filename << std::endl;
  stats.writeToFile(stats_filename);
  return stats;
}

MphTable::Stats MphTable::stats(const std::string& prefix) {
  return Stats::readFromFile(getNameOfStatsFile(prefix));
}

}  // namespace internal
}  // namespace multimap
