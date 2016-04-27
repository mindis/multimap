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

#include "multimap/thirdparty/mt/assert.hpp"
#include "multimap/thirdparty/mt/check.hpp"
#include "multimap/thirdparty/mt/fileio.hpp"

namespace bfs = boost::filesystem;

namespace multimap {
namespace internal {

const char* Descriptor::toString(int map_type) {
  switch (map_type) {
    case Descriptor::TYPE_MAP:
      return "Map";
    case Descriptor::TYPE_IMMUTABLE_MAP:
      return "ImmutableMap";
    default:
      mt::fail("Invalid argument: %d", map_type);
      return "";
  }
}

Descriptor Descriptor::readFromDirectory(const bfs::path& directory) {
  Descriptor descriptor;
  mt::check(tryReadFromDirectory(directory, &descriptor),
            "Reading descriptor from %s failed", directory.c_str());
  return descriptor;
}

bool Descriptor::tryReadFromDirectory(const bfs::path& directory,
                                      Descriptor* output) {
  const bfs::path file_path = directory / getFileName();
  const mt::AutoCloseFile stream = mt::fopenIfExists(file_path, "r");
  if (!stream) return false;
  mt::freadAll(stream.get(), output, sizeof *output);
  Version::checkCompatibility(output->major_version, output->minor_version);
  return true;
}

void Descriptor::writeToDirectory(const bfs::path& directory) const {
  MT_REQUIRE_NOT_ZERO(num_partitions);
  MT_REQUIRE_TRUE(map_type == TYPE_MAP || map_type == TYPE_IMMUTABLE_MAP);
  const auto stream = mt::fopen(directory / getFileName(), "w");
  mt::fwriteAll(stream.get(), this, sizeof *this);
}

std::string Descriptor::getFilePrefix() { return "multimap"; }

std::string Descriptor::getFileName() {
  return getFilePrefix() + ".descriptor";
}

}  // namespace internal
}  // namespace multimap
