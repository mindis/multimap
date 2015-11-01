// This file is part of the Multimap library.  http://multimap.io
//
// Copyright (C) 2015  Martin Trenkmann
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

#include "multimap/internal/System.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cassert>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <boost/filesystem/operations.hpp>
#include "multimap/thirdparty/mt.hpp"

namespace multimap {
namespace internal {

std::pair<boost::filesystem::path, int> System::getTempfile() {
  char pattern[] = "/tmp/system-tempfile-XXXXXX";
  const auto result = ::mkstemp(pattern);
  assert(result != -1);
  return std::make_pair(pattern, result);
}

std::ostream& System::log() { return log(std::cout); }

std::ostream& System::log(std::ostream& stream) { return log("", stream); }

std::ostream& System::log(const std::string& prefix) {
  return log(prefix, std::cout);
}

std::ostream& System::log(const std::string& prefix, std::ostream& stream) {
  printTimestamp(stream) << ' ';
  if (!prefix.empty()) {
    stream << prefix << ": ";
  }
  return stream;
}

std::ostream& System::printTimestamp(std::ostream& stream) {
  const auto time_since_epoch = std::time(nullptr);
  assert(time_since_epoch != -1);
  struct tm broken_down_time;
  ::localtime_r(&time_since_epoch, &broken_down_time);

  char old_fill_char = stream.fill('0');

  stream << broken_down_time.tm_year + 1900;
  stream << '-' << std::setw(2) << broken_down_time.tm_mon + 1;
  stream << '-' << std::setw(2) << broken_down_time.tm_mday;
  stream << ' ' << std::setw(2) << broken_down_time.tm_hour;
  stream << ':' << std::setw(2) << broken_down_time.tm_min;
  stream << ':' << std::setw(2) << broken_down_time.tm_sec;

  stream.fill(old_fill_char);
  return stream;
}

void System::close(std::FILE* stream) {
  const auto result = std::fclose(stream);
  assert(result == 0);
}

std::uint64_t System::offset(std::FILE* stream) {
  const auto result = std::ftell(stream);
  assert(result != -1);
  return result;
}

void System::seek(std::FILE* stream, std::int64_t offset, int origin) {
  const auto result = std::fseek(stream, offset, origin);
  assert(result == 0);
}

void System::read(std::FILE* stream, void* buf, std::size_t count) {
  const auto nbytes = std::fread(buf, sizeof(char), count, stream);
  assert(nbytes == count);
}

void System::write(std::FILE* stream, const void* buf, std::size_t count) {
  const auto nbytes = std::fwrite(buf, sizeof(char), count, stream);
  assert(nbytes == count);
}

const std::string System::DirectoryLockGuard::DEFAULT_FILENAME = ".lock";

System::DirectoryLockGuard::DirectoryLockGuard() {}

System::DirectoryLockGuard::DirectoryLockGuard(
    const boost::filesystem::path& directory) {
  lock(directory);
}

System::DirectoryLockGuard::DirectoryLockGuard(
    const boost::filesystem::path& directory, const std::string filename) {
  lock(directory, filename);
}

System::DirectoryLockGuard::DirectoryLockGuard(DirectoryLockGuard&& other)
    : directory_(other.directory_), filename_(other.filename_) {
  other.directory_.clear();
  other.filename_.clear();
}

System::DirectoryLockGuard& System::DirectoryLockGuard::operator=(
    DirectoryLockGuard&& other) {
  if (&other != this) {
    directory_ = other.directory_;
    filename_ = other.filename_;
    other.directory_.clear();
    other.filename_.clear();
  }
  return *this;
}

System::DirectoryLockGuard::~DirectoryLockGuard() {
  if (!directory_.empty()) {
    mt::check(
        std::remove((directory_ / filename_).c_str()) == 0,
        "DirectoryLockGuard: Could not unlock directory '%s' because it is "
        "not locked.",
        directory_.c_str());
  }
}

void System::DirectoryLockGuard::lock(
    const boost::filesystem::path& directory) {
  lock(directory, DEFAULT_FILENAME);
}

void System::DirectoryLockGuard::lock(const boost::filesystem::path& directory,
                                      const std::string filename) {
  mt::check(directory_.empty(), "DirectoryLockGuard: Already locked.");
  const mt::AutoCloseFile file(std::fopen((directory / filename).c_str(), "w"));
  mt::check(file.get(),
            "DirectoryLockGuard: Could not lock directory '%s' because it is "
            "already locked.",
            directory.c_str());
  directory_ = directory;
  filename_ = filename;
}

const boost::filesystem::path& System::DirectoryLockGuard::path() const {
  return directory_;
}

const std::string& System::DirectoryLockGuard::filename() const {
  return filename_;
}

}  // namespace internal
}  // namespace multimap
