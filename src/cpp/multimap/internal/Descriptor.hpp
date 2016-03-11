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
#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/Version.hpp"

namespace multimap {
namespace internal {

const std::string& getCommonFilePrefix();
const std::string& getNameOfDescriptorFile();
const std::string& getNameOfLockFile();

struct Descriptor {
  enum Type { NONE, MAP, IMMUTABLE_MAP };

  uint64_t major_version = Version::MAJOR;
  uint64_t minor_version = Version::MINOR;
  uint64_t num_partitions = 0;
  uint64_t type = NONE;

  static std::string toString(Type type);

  static Descriptor readFromDirectory(const boost::filesystem::path& directory,
                                      Type expected_type);

  static Descriptor readFromFile(const boost::filesystem::path& filename,
                                 Type expected_type);

  void writeToDirectory(const boost::filesystem::path& directory) const;

  void writeToFile(const boost::filesystem::path& filename) const;
};

static_assert(mt::hasExpectedSize<Descriptor>(32, 32),
              "struct Descriptor does not have expected size");

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_DESCRIPTOR_HPP_INCLUDED
