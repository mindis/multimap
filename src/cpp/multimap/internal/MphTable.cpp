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
                                const Options& options) {}

std::unique_ptr<Iterator> MphTable::get(const Bytes& key) const {}

Stats MphTable::getStats() const {}

std::string MphTable::getNameOfMphfFile(const std::string& prefix) {}
std::string MphTable::getNameOfDataFile(const std::string& prefix) {}
std::string MphTable::getNameOfListsFile(const std::string& prefix) {}
std::string MphTable::getNameOfStatsFile(const std::string& prefix) {}

}  // namespace internal
}  // namespace multimap
