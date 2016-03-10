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

#include "multimap/Version.hpp"

namespace multimap {

void Version::checkCompatibility(int extern_major, int extern_minor) {
  mt::Check::isTrue(
      isCompatible(extern_major, extern_minor),
      "Version check failed. "
      "Please install Multimap version %i.x where x is at least %i.",
      extern_major, extern_minor);
}

bool Version::isCompatible(int extern_major, int extern_minor,
                           int library_major, int library_minor) {
  if (extern_major != library_major) return false;
  if (extern_minor > library_minor) return false;
  return true;
}

const char* Meta::DEFAULT_FILENAME = "multimap.meta";

Meta Meta::readFromDirectory(const boost::filesystem::path& directory,
                             const char* filename) {
  return readFromFile(directory / filename);
}

Meta Meta::readFromFile(const boost::filesystem::path& filename) {
  Meta meta;
  const auto stream = mt::open(filename, "r");
  mt::read(stream.get(), &meta, sizeof meta);
  return meta;
}

void Meta::writeToDirectory(const boost::filesystem::path& directory,
                            const char* filename) const {
  writeToFile(directory / filename);
}

void Meta::writeToFile(const boost::filesystem::path& filename) const {
  const auto stream = mt::open(filename, "w");
  mt::write(stream.get(), this, sizeof *this);
}

}  // namespace multimap
