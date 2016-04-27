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

const boost::filesystem::path TMPDIR = "/tmp";

TEST(Descriptor, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<Descriptor>::value);
}

TEST(Descriptor, IsCopyConstructibleAndAssignable) {
  ASSERT_TRUE(std::is_copy_constructible<Descriptor>::value);
  ASSERT_TRUE(std::is_copy_assignable<Descriptor>::value);
}

TEST(Descriptor, DefaultConstructedHasProperState) {
  ASSERT_EQ(0, Descriptor().map_type);
  ASSERT_EQ(0, Descriptor().num_partitions);
  ASSERT_EQ(Version::MAJOR, Descriptor().major_version);
  ASSERT_EQ(Version::MINOR, Descriptor().minor_version);
  ASSERT_FALSE(Descriptor().getFilePrefix().empty());
  ASSERT_FALSE(Descriptor().getFileName().empty());
}

TEST(Descriptor, WriteThrowsWithDefaultConstructed) {
  ASSERT_THROW(Descriptor().writeToDirectory(TMPDIR), mt::AssertionError);
}

TEST(Descriptor, WriteThrowsIfNumPartitionsIsZero) {
  Descriptor descriptor;
  descriptor.num_partitions = 0;
  ASSERT_THROW(descriptor.writeToDirectory(TMPDIR), mt::AssertionError);
}

TEST(Descriptor, WriteThrowsIfMapTypeIsZero) {
  Descriptor descriptor;
  descriptor.map_type = 0;
  ASSERT_THROW(descriptor.writeToDirectory(TMPDIR), mt::AssertionError);
}

TEST(Descriptor, WriteAndReadSucceedsForValidDescriptor) {
  Descriptor expected;
  expected.num_partitions = 1;
  expected.map_type = Descriptor::TYPE_MAP;
  ASSERT_NO_THROW(expected.writeToDirectory(TMPDIR));

  Descriptor actual = Descriptor::readFromDirectory(TMPDIR);
  ASSERT_EQ(expected.map_type, actual.map_type);
  ASSERT_EQ(expected.num_partitions, actual.num_partitions);
  ASSERT_EQ(expected.major_version, actual.major_version);
  ASSERT_EQ(expected.minor_version, actual.minor_version);

  expected.map_type = Descriptor::TYPE_IMMUTABLE_MAP;
  ASSERT_NO_THROW(expected.writeToDirectory(TMPDIR));

  actual = Descriptor::readFromDirectory(TMPDIR);
  ASSERT_EQ(expected.map_type, actual.map_type);
  ASSERT_EQ(expected.num_partitions, actual.num_partitions);
  ASSERT_EQ(expected.major_version, actual.major_version);
  ASSERT_EQ(expected.minor_version, actual.minor_version);
}

TEST(Descriptor, TryReadReturnsFalseIfDirectoryDoesNotExist) {
  Descriptor descriptor;
  const char* tmpdir = "/abc";
  ASSERT_FALSE(boost::filesystem::exists(tmpdir));
  ASSERT_FALSE(Descriptor::tryReadFromDirectory(tmpdir, &descriptor));
}

TEST(Descriptor, TryReadReturnsFalseIfDirectoryDoesNotContainDescriptor) {
  Descriptor descriptor;
  boost::filesystem::remove(TMPDIR / Descriptor::getFileName());
  ASSERT_FALSE(Descriptor::tryReadFromDirectory(TMPDIR, &descriptor));
}

TEST(Descriptor, TryReadReturnsTrueIfDirectoryContainsDescriptor) {
  Descriptor descriptor;
  descriptor.num_partitions = 1;
  descriptor.map_type = Descriptor::TYPE_MAP;
  descriptor.writeToDirectory(TMPDIR);

  Descriptor read_descriptor;
  ASSERT_TRUE(Descriptor::tryReadFromDirectory(TMPDIR, &read_descriptor));
  ASSERT_EQ(descriptor.map_type, read_descriptor.map_type);
  ASSERT_EQ(descriptor.num_partitions, read_descriptor.num_partitions);
  ASSERT_EQ(descriptor.major_version, read_descriptor.major_version);
  ASSERT_EQ(descriptor.minor_version, read_descriptor.minor_version);
}

}  // namespace internal
}  // namespace multimap
