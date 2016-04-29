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

#include <type_traits>
#include "gmock/gmock.h"
#include "multimap/Version.h"

namespace multimap {

TEST(VersionTest, IsNotDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<Version>::value);
}

TEST(VersionTest, DifferentMajorVersionsAreNotCompatible) {
  int extern_major = 0;
  int library_major = 0;
  ASSERT_TRUE(Version::isCompatible(extern_major, 0, library_major, 0));

  extern_major = 0;
  library_major = 1;
  ASSERT_FALSE(Version::isCompatible(extern_major, 0, library_major, 0));

  extern_major = 1;
  library_major = 0;
  ASSERT_FALSE(Version::isCompatible(extern_major, 0, library_major, 0));
}

TEST(VersionTest, SameExternMinorVersionIsCompatibleWithLibrary) {
  int extern_minor = 0;
  int library_minor = 0;
  ASSERT_TRUE(Version::isCompatible(0, extern_minor, 0, library_minor));
}

TEST(VersionTest, LowerExternMinorVersionIsCompatibleWithLibrary) {
  int extern_minor = 0;
  int library_minor = 1;
  ASSERT_TRUE(Version::isCompatible(0, extern_minor, 0, library_minor));
}

TEST(VersionTest, HigherExternMinorVersionIsNotCompatibleWithLibrary) {
  int extern_minor = 1;
  int library_minor = 0;
  ASSERT_FALSE(Version::isCompatible(0, extern_minor, 0, library_minor));
}

}  // namespace multimap
