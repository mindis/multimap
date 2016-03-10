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

#ifndef MULTIMAP_VERSION_HPP_INCLUDED
#define MULTIMAP_VERSION_HPP_INCLUDED

#include <boost/filesystem/path.hpp>
#include "multimap/thirdparty/mt/mt.hpp"

namespace multimap {

struct Version {
  static const int MAJOR = 0;
  static const int MINOR = 6;
  static const int PATCH = 0;

  static void checkCompatibility(int extern_major, int extern_minor);

  static bool isCompatible(int extern_major, int extern_minor,
                           int lib_major = MAJOR, int lib_minor = MINOR);

  Version() = delete;
};

namespace internal {

const std::string& getCommonFilePrefix();
const std::string& getNameOfLockFile();
const std::string& getNameOfMetaFile();

struct Meta {
  enum Type { NONE, MAP, IMMUTABLE_MAP };

  uint64_t major_version = Version::MAJOR;
  uint64_t minor_version = Version::MINOR;
  uint64_t num_partitions = 0;
  uint64_t type = NONE;

  static Meta readFromDirectory(
      const boost::filesystem::path& directory,
      const std::string& filename = getNameOfMetaFile());

  static Meta readFromFile(const boost::filesystem::path& filename);

  void writeToDirectory(
      const boost::filesystem::path& directory,
      const std::string& filename = getNameOfMetaFile()) const;

  void writeToFile(const boost::filesystem::path& filename) const;
};

static_assert(mt::hasExpectedSize<Meta>(32, 32),
              "struct Meta does not have expected size");

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_VERSION_HPP_INCLUDED
