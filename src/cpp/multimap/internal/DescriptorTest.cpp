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
#include <boost/filesystem/operations.hpp>
#include "multimap/internal/Descriptor.hpp"
#include "multimap/thirdparty/mt/assert.hpp"

namespace multimap {
namespace internal {

const char* TMPDIR = "/tmp";

TEST(Descriptor, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<Descriptor>::value);
}

TEST(Descriptor, IsCopyConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_copy_constructible<Descriptor>::value);
  ASSERT_TRUE(std::is_copy_assignable<Descriptor>::value);
}

TEST(Descriptor, DefaultConstructedHasProperState) {
  ASSERT_FALSE(Descriptor().getFileName().empty());
  ASSERT_FALSE(Descriptor().getFilePrefix().empty());
  ASSERT_FALSE(Descriptor().getFullFileName("").empty());
  ASSERT_FALSE(Descriptor().getFullFilePrefix("").empty());
  ASSERT_EQ(Version::MAJOR, Descriptor().major_version);
  ASSERT_EQ(Version::MINOR, Descriptor().minor_version);
  ASSERT_EQ(0, Descriptor().num_partitions);
  ASSERT_EQ(0, Descriptor().map_type);
}

TEST(Descriptor, WriteThrowsWithDefaultConstructed) {
  ASSERT_THROW(Descriptor().writeToDirectory(TMPDIR), mt::AssertionError);
}

TEST(Descriptor, WriteThrowsIfNumPartitionsIsZero) {
  Descriptor descriptor;
  descriptor.num_partitions = 0;
  descriptor.map_type = Descriptor::MAP;
  ASSERT_THROW(descriptor.writeToDirectory(TMPDIR), mt::AssertionError);
}

TEST(Descriptor, WriteThrowsIfMapTypeIsZero) {
  Descriptor descriptor;
  descriptor.map_type = 0;
  descriptor.num_partitions = 1;
  ASSERT_THROW(descriptor.writeToDirectory(TMPDIR), mt::AssertionError);
}

TEST(Descriptor, WriteAndReadSucceedsForValidDescriptor) {
  Descriptor descriptor;
  descriptor.num_partitions = 1;
  descriptor.map_type = Descriptor::MAP;
  ASSERT_NO_THROW(descriptor.writeToDirectory(TMPDIR));

  Descriptor read_descriptor = Descriptor::readFromDirectory(TMPDIR);
  ASSERT_EQ(descriptor.major_version, read_descriptor.major_version);
  ASSERT_EQ(descriptor.minor_version, read_descriptor.minor_version);
  ASSERT_EQ(descriptor.num_partitions, read_descriptor.num_partitions);
  ASSERT_EQ(descriptor.map_type, read_descriptor.map_type);

  descriptor.map_type = Descriptor::IMMUTABLE_MAP;
  ASSERT_NO_THROW(descriptor.writeToDirectory(TMPDIR));

  read_descriptor = Descriptor::readFromDirectory(TMPDIR);
  ASSERT_EQ(descriptor.major_version, read_descriptor.major_version);
  ASSERT_EQ(descriptor.minor_version, read_descriptor.minor_version);
  ASSERT_EQ(descriptor.num_partitions, read_descriptor.num_partitions);
  ASSERT_EQ(descriptor.map_type, read_descriptor.map_type);
}

TEST(Descriptor, TryReadReturnsFalseIfDirectoryDoesNotExist) {
  Descriptor descriptor;
  const char* tmpdir = "/abc";
  ASSERT_FALSE(boost::filesystem::exists(tmpdir));
  ASSERT_FALSE(Descriptor::tryReadFromDirectory(tmpdir, &descriptor));
}

TEST(Descriptor, TryReadReturnsFalseIfDirectoryDoesNotContainDescriptor) {
  Descriptor descriptor;
  boost::filesystem::remove(Descriptor::getFullFileName(TMPDIR));
  ASSERT_FALSE(Descriptor::tryReadFromDirectory(TMPDIR, &descriptor));
}

TEST(Descriptor, TryReadReturnsTrueIfDirectoryContainsDescriptor) {
  Descriptor descriptor;
  descriptor.num_partitions = 1;
  descriptor.map_type = Descriptor::MAP;
  descriptor.writeToDirectory(TMPDIR);

  Descriptor read_descriptor;
  ASSERT_TRUE(Descriptor::tryReadFromDirectory(TMPDIR, &read_descriptor));
  ASSERT_EQ(descriptor.major_version, read_descriptor.major_version);
  ASSERT_EQ(descriptor.minor_version, read_descriptor.minor_version);
  ASSERT_EQ(descriptor.num_partitions, read_descriptor.num_partitions);
  ASSERT_EQ(descriptor.map_type, read_descriptor.map_type);
}

TEST(Descriptor, ValidateThrowsIfExpectedTypeDoesNotMatch) {
  Descriptor descriptor;
  descriptor.map_type = Descriptor::MAP;
  ASSERT_THROW(Descriptor::validate(descriptor, Descriptor::IMMUTABLE_MAP),
               std::runtime_error);

  descriptor.map_type = Descriptor::IMMUTABLE_MAP;
  ASSERT_THROW(Descriptor::validate(descriptor, Descriptor::MAP),
               std::runtime_error);
}

TEST(Descriptor, ValidateThrowsIfMajorVersionIsNotCompatible) {
  Descriptor descriptor;
  descriptor.map_type = Descriptor::MAP;
  descriptor.major_version = Version::MAJOR - 1;
  ASSERT_THROW(Descriptor::validate(descriptor, Descriptor::MAP),
               std::runtime_error);

  descriptor.major_version = Version::MAJOR + 1;
  ASSERT_THROW(Descriptor::validate(descriptor, Descriptor::MAP),
               std::runtime_error);
}

TEST(Descriptor, ValidateThrowsIfMinorVersionIsNotCompatible) {
  Descriptor descriptor;
  descriptor.map_type = Descriptor::MAP;
  descriptor.minor_version = Version::MINOR - 1;
  ASSERT_NO_THROW(Descriptor::validate(descriptor, Descriptor::MAP));

  descriptor.minor_version = Version::MINOR + 1;
  ASSERT_THROW(Descriptor::validate(descriptor, Descriptor::MAP),
               std::runtime_error);
}

TEST(Descriptor, ValidateDoesNotThrowIfVersionIsCompatible) {
  Descriptor descriptor;
  descriptor.map_type = Descriptor::MAP;
  ASSERT_NO_THROW(Descriptor::validate(descriptor, Descriptor::MAP));

  descriptor.minor_version = Version::MINOR - 1;
  ASSERT_NO_THROW(Descriptor::validate(descriptor, Descriptor::MAP));
}

}  // namespace internal
}  // namespace multimap
