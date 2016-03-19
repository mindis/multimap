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

struct Descriptor {
  enum Type { NONE, MAP, IMMUTABLE_MAP };

  uint64_t major_version = Version::MAJOR;
  uint64_t minor_version = Version::MINOR;
  uint64_t num_partitions = 0;
  uint64_t type = NONE;

  static std::string getFilename();

  static std::string getFilePrefix();

  static std::string getFullFilename(const boost::filesystem::path& base);

  static std::string getFullFilePrefix(const boost::filesystem::path& base);

  static Descriptor readFromDirectory(const boost::filesystem::path& directory,
                                      Type expected_type);

  static bool tryReadFromDirectory(const boost::filesystem::path& directory,
                                   Type expected_type, Descriptor* output);

  void writeToDirectory(const boost::filesystem::path& directory) const;

  static std::string toString(Type type);

  Descriptor() = default;
};

static_assert(mt::hasExpectedSize<Descriptor>(32, 32),
              "struct Descriptor does not have expected size");

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_DESCRIPTOR_HPP_INCLUDED
