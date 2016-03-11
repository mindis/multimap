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

#include "multimap/internal/Descriptor.hpp"

namespace multimap {
namespace internal {

const std::string& getCommonFilePrefix() {
  static const std::string prefix = "multimap";
  return prefix;
}

const std::string& getNameOfLockFile() {
  static const std::string filename = getCommonFilePrefix() + ".lock";
  return filename;
}

const std::string& getNameOfMetaFile() {
  static const std::string filename = getCommonFilePrefix() + ".desc";
  return filename;
}

Descriptor Descriptor::readFromDirectory(
    const boost::filesystem::path& directory, const std::string& filename) {
  return readFromFile(directory / filename);
}

Descriptor Descriptor::readFromFile(const boost::filesystem::path& filename) {
  Descriptor meta;
  const auto stream = mt::open(filename, "r");
  mt::read(stream.get(), &meta, sizeof meta);
  return meta;
}

void Descriptor::writeToDirectory(const boost::filesystem::path& directory,
                                  const std::string& filename) const {
  writeToFile(directory / filename);
}

void Descriptor::writeToFile(const boost::filesystem::path& filename) const {
  const auto stream = mt::open(filename, "w");
  mt::write(stream.get(), this, sizeof *this);
}

}  // namespace internal
}  // namespace multimap
