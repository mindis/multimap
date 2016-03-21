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

namespace {

std::string mapTypeToString(uint32_t map_type) {
  switch (map_type) {
    case Descriptor::MAP:
      return "Map";
    case Descriptor::IMMUTABLE_MAP:
      return "ImmutableMap";
    default:
      mt::fail("mapTypeToString(%u) is undefined", map_type);
      return "";
  }
}

}  // namespace

std::string Descriptor::getFilename() { return getFilePrefix() + ".desc"; }

std::string Descriptor::getFilePrefix() { return "multimap"; }

std::string Descriptor::getFullFilename(const boost::filesystem::path& base) {
  return (base / getFilename()).string();
}

std::string Descriptor::getFullFilePrefix(const boost::filesystem::path& base) {
  return (base / getFilePrefix()).string();
}

Descriptor Descriptor::readFromDirectory(
    const boost::filesystem::path& directory) {
  Descriptor descriptor;
  mt::check(tryReadFromDirectory(directory, &descriptor),
            "Reading descriptor from '%s' failed", directory.c_str());
  return descriptor;
}

bool Descriptor::tryReadFromDirectory(const boost::filesystem::path& directory,
                                      Descriptor* output) {
  const auto filename = directory / getFilename();
  const auto stream = mt::tryOpen(filename, "r");
  if (stream.get()) {
    mt::read(stream.get(), output, sizeof *output);

    return true;
  }
  return false;
}

void Descriptor::writeToDirectory(
    const boost::filesystem::path& directory) const {
  MT_REQUIRE_NOT_ZERO(num_partitions);
  MT_REQUIRE_TRUE(map_type == MAP || map_type == IMMUTABLE_MAP);
  const auto stream = mt::open(directory / getFilename(), "w");
  mt::write(stream.get(), this, sizeof *this);
}

void Descriptor::validate(const Descriptor& descriptor, int expected_map_type) {
  Version::checkCompatibility(descriptor.major_version,
                              descriptor.minor_version);
  mt::Check::isEqual(expected_map_type, descriptor.map_type,
                     "Validation of descriptor failed. Expected type '%s' but "
                     "the actual type was '%s'",
                     mapTypeToString(expected_map_type).c_str(),
                     mapTypeToString(descriptor.map_type).c_str());
}

}  // namespace internal
}  // namespace multimap
