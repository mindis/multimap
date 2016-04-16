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

#ifndef MULTIMAP_INTERNAL_DESCRIPTOR_HPP_INCLUDED
#define MULTIMAP_INTERNAL_DESCRIPTOR_HPP_INCLUDED

#include <boost/filesystem/path.hpp>
#include "multimap/thirdparty/mt/common.hpp"
#include "multimap/Version.hpp"

namespace multimap {
namespace internal {

struct Descriptor {
  static const int MAP = 1;
  static const int IMMUTABLE_MAP = 2;

  int major_version = Version::MAJOR;
  int minor_version = Version::MINOR;
  int num_partitions = 0;
  int map_type = 0;

  static std::string getFileName();

  static std::string getFilePrefix();

  static std::string getFullFileName(const boost::filesystem::path& base);

  static std::string getFullFilePrefix(const boost::filesystem::path& base);

  static Descriptor readFromDirectory(const boost::filesystem::path& directory);

  static bool tryReadFromDirectory(const boost::filesystem::path& directory,
                                   Descriptor* output);

  void writeToDirectory(const boost::filesystem::path& directory) const;

  static void validate(const Descriptor& descriptor, int expected_map_type);

  Descriptor() = default;
};

MT_ASSERT_SIZEOF(Descriptor, 16, 16);

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_DESCRIPTOR_HPP_INCLUDED
