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

std::string Descriptor::getFilename() { return getFilePrefix() + ".desc"; }

std::string Descriptor::getFilePrefix() { return "multimap"; }

std::string Descriptor::getFullFilename(const boost::filesystem::path& base) {
  return (base / getFilename()).string();
}

std::string Descriptor::getFullFilePrefix(const boost::filesystem::path& base) {
  return (base / getFilePrefix()).string();
}

Descriptor Descriptor::readFromDirectory(
    const boost::filesystem::path& directory, Type expected_type) {
  Descriptor descriptor;
  mt::check(tryReadFromDirectory(directory, expected_type, &descriptor),
            "Reading descriptor from '%s' failed", directory.c_str());
  return descriptor;
}

bool Descriptor::tryReadFromDirectory(const boost::filesystem::path& directory,
                                      Type expected_type, Descriptor* output) {
  const auto filename = directory / getFilename();
  const auto stream = mt::tryOpen(filename, "r");
  if (stream.get()) {
    mt::read(stream.get(), output, sizeof *output);
    Version::checkCompatibility(output->major_version, output->minor_version);
    mt::check(expected_type == output->type,
              "Attempt to open an instance of type '%s' from '%s' but the "
              "actual type found was '%s'",
              toString(expected_type).c_str(), filename.parent_path().c_str(),
              toString(static_cast<Type>(output->type)).c_str());
    return true;
  }
  return false;
}

void Descriptor::writeToDirectory(
    const boost::filesystem::path& directory) const {
  MT_REQUIRE_TRUE(type == MAP || type == IMMUTABLE_MAP);
  const auto stream = mt::open(directory / getFilename(), "w");
  mt::write(stream.get(), this, sizeof *this);
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
      MT_FAIL("Reached default branch in switch statement");
  }
  return "";
}

}  // namespace internal
}  // namespace multimap
