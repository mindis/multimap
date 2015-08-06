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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <boost/filesystem/operations.hpp>
#include "multimap/internal/Check.hpp"
#include "multimap/Bytes.hpp"

namespace multimap {
namespace internal {

std::pair<boost::filesystem::path, int> System::Tempfile() {
  char pattern[] = "/tmp/system-tempfile-XXXXXX";
  const auto result = ::mkstemp(pattern);
  assert(result != -1);
  return std::make_pair(pattern, result);
}

std::ostream& System::Log() { return Log(std::cout); }

std::ostream& System::Log(std::ostream& stream) { return Log("", stream); }

std::ostream& System::Log(const std::string& prefix) {
  return Log(prefix, std::cout);
}

std::ostream& System::Log(const std::string& prefix, std::ostream& stream) {
  PrintTimestamp(stream) << ' ';
  if (!prefix.empty()) {
    stream << prefix << ": ";
  }
  return stream;
}

std::ostream& System::PrintTimestamp(std::ostream& stream) {
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

void System::Close(std::FILE* stream) {
  const auto result = std::fclose(stream);
  assert(result == 0);
}

std::uint64_t System::Offset(std::FILE* stream) {
  const auto result = std::ftell(stream);
  assert(result != -1);
  return result;
}

void System::Seek(std::FILE* stream, std::uint64_t offset) {
  const auto result = std::fseek(stream, offset, SEEK_SET);
  assert(result == 0);
}

void System::Read(std::FILE* stream, void* buf, std::size_t count) {
  const auto nbytes = std::fread(buf, sizeof(byte), count, stream);
  assert(nbytes == count);
}

void System::Write(std::FILE* stream, const void* buf, std::size_t count) {
  const auto nbytes = std::fwrite(buf, sizeof(byte), count, stream);
  assert(nbytes == count);
}

int System::Open(const boost::filesystem::path& filepath,
                 bool create_if_missing) {
  if (create_if_missing) {
    const auto mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    return ::open64(filepath.c_str(), O_RDWR | O_NOATIME | O_CREAT, mode);
  }
  return ::open64(filepath.c_str(), O_RDWR | O_NOATIME);
}

bool System::Create(const boost::filesystem::path& filepath) {
  if (boost::filesystem::exists(filepath)) return false;
  const auto fd = Open(filepath, true);
  assert(fd != -1);
  Close(fd);
  return true;
}

bool System::Remove(const boost::filesystem::path& filepath) {
  return boost::filesystem::remove(filepath);
}

void System::Close(int fd) {
  const auto result = ::close(fd);
  assert(result != -1);
}

std::uint64_t System::Offset(int fd) {
  const auto result = ::lseek64(fd, 0, SEEK_CUR);
  assert(result != -1);
  return result;
}

void System::Seek(int fd, std::uint64_t offset) {
  const auto result = ::lseek64(fd, offset, SEEK_SET);
  assert(result != -1);
}

void System::Read(int fd, void* buf, std::size_t count) {
  const auto result = ::read(fd, buf, count);
  assert(result != -1);
  const auto nbytes = static_cast<std::size_t>(result);
  assert(nbytes == count);
}

void System::Read(int fd, void* buf, std::size_t count, std::uint64_t offset) {
  const auto result = ::pread64(fd, buf, count, offset);
  assert(result != -1);
  const auto nbytes = static_cast<std::size_t>(result);
  assert(nbytes == count);
}

void System::Write(int fd, const void* buf, std::size_t count) {
  const auto result = ::write(fd, buf, count);
  assert(result != -1);
  const auto nbytes = static_cast<std::size_t>(result);
  assert(nbytes == count);
}

void System::Write(int fd, const void* buf, std::size_t count,
                   std::uint64_t offset) {
  const auto result = ::pwrite64(fd, buf, count, offset);
  assert(result != -1);
  const auto nbytes = static_cast<std::size_t>(result);
  assert(nbytes == count);
}

const std::size_t System::Batch::kMaxSize = sysconf(_SC_IOV_MAX);

bool System::Batch::TryAdd(const void* data, std::size_t size) {
  if (items_.size() < kMaxSize) {
    items_.push_back(iovec{const_cast<void*>(data), size});
    return true;
  }
  return false;
}

void System::Batch::Add(const void* data, std::size_t size) {
  assert(items_.size() < kMaxSize);
  items_.push_back(iovec{const_cast<void*>(data), size});
}

std::uint64_t System::Batch::Write(int fd) const {
  std::int64_t num_bytes_total = 0;
  for (const auto& item : items_) {
    num_bytes_total += item.iov_len;
  }

  const auto result = ::writev(fd, items_.data(), items_.size());
  assert(result != -1);
  assert(result == num_bytes_total);
  return num_bytes_total;
}

std::size_t System::Batch::size() const { return items_.size(); }

bool System::Batch::empty() const { return items_.empty(); }

bool System::Batch::full() const { return items_.size() == kMaxSize; }

void System::Batch::clear() { items_.clear(); }

const std::string System::DirectoryLockGuard::kDefaultFilename(".lock");

System::DirectoryLockGuard::DirectoryLockGuard() {}

System::DirectoryLockGuard::DirectoryLockGuard(
    const boost::filesystem::path& directory) {
  Lock(directory);
}

System::DirectoryLockGuard::DirectoryLockGuard(
    const boost::filesystem::path& directory, const std::string filename) {
  Lock(directory, filename);
}

System::DirectoryLockGuard::~DirectoryLockGuard() {
  if (!directory_.empty()) {
    Check(Remove(directory_ / filename_),
          "DirectoryLockGuard: Could not unlock directory '%s' because it is "
          "not locked.",
          directory_.c_str());
  }
}

void System::DirectoryLockGuard::Lock(
    const boost::filesystem::path& directory) {
  Lock(directory, kDefaultFilename);
}

void System::DirectoryLockGuard::Lock(const boost::filesystem::path& directory,
                                      const std::string filename) {
  Check(directory_.empty(), "DirectoryLockGuard: Already locked.");
  Check(Create(directory / filename),
        "DirectoryLockGuard: Could not lock directory '%s' because it is "
        "already locked.",
        directory.c_str());
  directory_ = directory;
  filename_ = filename;
}

const boost::filesystem::path& System::DirectoryLockGuard::directory() const {
  return directory_;
}

const std::string& System::DirectoryLockGuard::filename() const {
  return filename_;
}

}  // namespace internal
}  // namespace multimap
