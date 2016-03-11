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

std::string Descriptor::toString(Type type) {
  switch (type) {
    case NONE:
      return "None";
    case MAP:
      return "Map";
    case IMMUTABLE_MAP:
      return "ImmutableMap";
    default:
      MT_FAIL("Default branch in switch statement reached");
  }
  return "";
}

Descriptor Descriptor::readFromDirectory(
    const boost::filesystem::path& directory, Type expected_type) {
  return readFromFile(directory / getNameOfMetaFile(), expected_type);
}

Descriptor Descriptor::readFromFile(const boost::filesystem::path& filename,
                                    Type expected_type) {
  Descriptor desc;
  const auto stream = mt::open(filename, "r");
  mt::read(stream.get(), &desc, sizeof desc);
  Version::checkCompatibility(desc.major_version, desc.minor_version);
  mt::Check::isEqual(expected_type, desc.type,
                     "Attempt to open an instance of type '%s' from '%s' but "
                     "the actual type found was '%s'",
                     toString(expected_type).c_str(),
                     filename.parent_path().c_str(),
                     toString(static_cast<Type>(desc.type)).c_str());
  return desc;
}

void Descriptor::writeToDirectory(
    const boost::filesystem::path& directory) const {
  writeToFile(directory / getNameOfMetaFile());
}

void Descriptor::writeToFile(const boost::filesystem::path& filename) const {
  MT_REQUIRE_TRUE(type == MAP || type == IMMUTABLE_MAP);
  const auto stream = mt::open(filename, "w");
  mt::write(stream.get(), this, sizeof *this);
}

}  // namespace internal
}  // namespace multimap
