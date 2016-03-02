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

#include <limits>
#include <unordered_map>
#include "multimap/internal/Arena.hpp"

namespace multimap {
namespace internal {

uint32_t MphTable::Limits::maxKeySize() {
  return std::numeric_limits<int32_t>::max();
}

uint32_t MphTable::Limits::maxValueSize() {
  return std::numeric_limits<int32_t>::max();
}

MphTable::MphTable(const boost::filesystem::path& prefix) {}

MphTable::~MphTable() {}

MphTable::Stats MphTable::build(const boost::filesystem::path& datafile,
                                const Options& options) {
  Arena arena;
  std::vector<char> key;
  std::vector<char> value;
  typedef std::vector<Bytes> List;
  std::unordered_map<Bytes, List> map;
  const auto stream = mt::fopen(datafile, "r");
  while (readBytes(stream.get(), &key) && readBytes(stream.get(), &value)) {
    auto iter = map.find(Bytes(key.data(), key.size()));
    if (iter == map.end()) {
      auto new_key_data = arena.allocate(key.size());
      std::memcpy(new_key_data, key.data(), key.size());
      iter = map.emplace(Bytes(new_key_data, key.size()), List()).first;
    }
    auto new_value_data = arena.allocate(value.size());
    std::memcpy(new_value_data, value.data(), value.size());
    iter->second.emplace_back(new_value_data, value.size());
  }
}

std::unique_ptr<Iterator> MphTable::get(const Bytes& key) const {}

Stats MphTable::getStats() const {}

std::string MphTable::getNameOfMphfFile(const std::string& prefix) {}
std::string MphTable::getNameOfDataFile(const std::string& prefix) {}
std::string MphTable::getNameOfListsFile(const std::string& prefix) {}
std::string MphTable::getNameOfStatsFile(const std::string& prefix) {}

}  // namespace internal
}  // namespace multimap
