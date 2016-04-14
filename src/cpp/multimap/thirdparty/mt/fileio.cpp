// This file is part of the MT library.
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

#include "mt/fileio.hpp"

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include "mt/check.hpp"

namespace mt {

std::vector<uint8_t> readAllBytes(const std::string& filename) {
  struct stat stats;
  const int result = stat(filename.c_str(), &stats);
  Check::isZero(result, "stat() failed for %s because of '%s'",
                filename.c_str(), errnostr());
  std::vector<uint8_t> bytes(stats.st_size);
  const AutoCloseFd fd = open(filename.c_str(), O_RDONLY);
  readAll(fd.get(), bytes.data(), bytes.size());
  return bytes;
}

std::vector<std::string> readAllLines(const std::string& filename) {
  std::ifstream stream(filename);
  check(stream.is_open(), "Could not open %s", filename.c_str());

  std::string line;
  std::vector<std::string> lines;
  while (std::getline(stream, line)) {
    lines.push_back(line);
  }
  return lines;
}

std::vector<std::string> listFiles(const std::string& directory,
                                   bool ignore_hidden) {
  DIR* stream = opendir(directory.c_str());
  Check::notNull(stream, "opendir() failed for %s because of '%s'",
                 directory.c_str(), errnostr());

  struct dirent* entry;
  std::vector<std::string> filenames;
  while ((entry = readdir(stream)) != nullptr) {
    if (entry->d_type == DT_REG) {
      if (ignore_hidden && entry->d_name[0] == '.') continue;
      filenames.emplace_back(entry->d_name);
    }
  }

  const int result = closedir(stream);
  Check::isZero(result, "closedir() failed because of '%s'", errnostr());
  return filenames;
}

const int AutoCloseFd::NOFD = -1;

AutoCloseFd::AutoCloseFd(int fd) : fd_(fd) {}

AutoCloseFd::AutoCloseFd(AutoCloseFd&& other) : fd_(other.release()) {}

AutoCloseFd::~AutoCloseFd() { reset(); }

AutoCloseFd& AutoCloseFd::operator=(AutoCloseFd&& other) {
  reset(other.release());
  return *this;
}

AutoCloseFd::operator bool() const noexcept { return fd_ != NOFD; }

int AutoCloseFd::release() {
  int fd = fd_;
  fd_ = NOFD;
  return fd;
}

void AutoCloseFd::reset(int fd) {
  if (fd_ != NOFD) {
    const auto result = ::close(fd_);
    Check::isZero(result, "close() failed because of '%s'", errnostr());
  }
  fd_ = fd;
}

AutoCloseFd open(const std::string& path, int flags) {
  AutoCloseFd fd = openIfExists(path, flags);
  check(bool(fd), "open() failed for %s because of '%s'", path.c_str(),
        errnostr());
  return fd;
}

AutoCloseFd open(const std::string& path, int flags, mode_t mode) {
  AutoCloseFd fd = openIfExists(path, flags, mode);
  check(bool(fd), "open() failed for %s because of '%s'", path.c_str(),
        errnostr());
  return fd;
}

AutoCloseFd openIfExists(const std::string& path, int flags) {
  return AutoCloseFd(::open(path.c_str(), flags));
}

AutoCloseFd openIfExists(const std::string& path, int flags, mode_t mode) {
  return AutoCloseFd(::open(path.c_str(), flags, mode));
}

AutoCloseFd creat(const std::string& path, mode_t mode) {
  AutoCloseFd fd(::creat(path.c_str(), mode));
  check(bool(fd), "creat() failed for %s because of '%s'", path.c_str(),
        errnostr());
  return fd;
}

void readAll(int fd, void* buf, size_t count) {
  check(readAllMaybe(fd, buf, count), "read() got less bytes than expected");
}

bool readAllMaybe(int fd, void* buf, size_t count) {
  return static_cast<size_t>(::read(fd, buf, count)) == count;
}

void preadAll(int fd, void* buf, size_t count, uint64_t offset) {
  Check::isEqual(count, ::pread(fd, buf, count, offset),
                 "pread() got less bytes than expected");
}

void writeAll(int fd, const void* buf, size_t count) {
  Check::isEqual(count, ::write(fd, buf, count),
                 "write() put less bytes than expected");
}

void pwriteAll(int fd, const void* buf, size_t count, uint64_t offset) {
  Check::isEqual(count, ::pwrite(fd, buf, count, offset),
                 "pwrite() put less bytes than expected");
}

uint64_t lseek(int fd, int64_t offset, int whence) {
  const off_t result = ::lseek(fd, offset, whence);
  Check::notEqual(-1, result, "lseek() failed because of '%s'", errnostr());
  return result;
}

uint64_t tell(int fd) { return lseek(fd, 0, SEEK_CUR); }

void ftruncate(int fd, uint64_t length) {
  Check::isZero(::ftruncate(fd, length), "ftruncate() failed because of '%s'",
                errnostr());
}

AutoCloseFile fopen(const std::string& path, const char* mode) {
  AutoCloseFile file = fopenIfExists(path, mode);
  check(bool(file), "fopen() failed for %s because of '%s'", path.c_str(),
        errnostr());
  return file;
}

AutoCloseFile fopenIfExists(const std::string& path, const char* mode) {
  return AutoCloseFile(std::fopen(path.c_str(), mode));
}

uint8_t fgetc(std::FILE* stream) {
  const int result = std::fgetc(stream);
  Check::notEqual(EOF, result, "fgetc() failed");
  return result;
}

bool fgetcMaybe(std::FILE* stream, uint8_t* byte) {
  const int result = std::fgetc(stream);
  if (result == EOF) return false;
  *byte = result;
  return true;
}

void fputc(std::FILE* stream, uint8_t byte) {
  const int result = std::fputc(byte, stream);
  Check::notEqual(EOF, result, "fputc() failed");
}

void freadAll(std::FILE* stream, void* buf, size_t count) {
  check(freadAllMaybe(stream, buf, count), "fread() failed");
}

bool freadAllMaybe(std::FILE* stream, void* buf, size_t count) {
  return std::fread(buf, sizeof(char), count, stream) == count;
}

void fwriteAll(std::FILE* stream, const void* buf, size_t count) {
  Check::isEqual(count, std::fwrite(buf, sizeof(char), count, stream),
                 "fwrite() failed");
}

void fseek(std::FILE* stream, int64_t offset, int whence) {
  Check::isZero(std::fseek(stream, offset, whence),
                "fseek() failed because of '%s'", errnostr());
}

uint64_t ftell(std::FILE* stream) {
  const auto offset = std::ftell(stream);
  Check::notEqual(-1, offset, "ftell() failed because of '%s'", errnostr());
  return offset;
}

const std::string DirectoryLockGuard::DEFAULT_FILENAME = ".lock";

DirectoryLockGuard::DirectoryLockGuard(const std::string& directory,
                                       const std::string& filename)
    : directory_(directory), filename_(filename) {
  const std::string abs_filename = directory + '/' + filename;
  struct stat file_stats;
  Check::isEqual(-1, ::stat(abs_filename.c_str(), &file_stats),
                 "Could not create %s because the file already exists",
                 abs_filename.c_str());
  std::ofstream stream(abs_filename);
  Check::isTrue(stream.is_open(),
                "Could not create lock file %s for unknown reason",
                abs_filename.c_str());
  stream << ::getpid();
}

DirectoryLockGuard::~DirectoryLockGuard() {
  if (!directory_.empty()) {
    std::remove((directory_ + '/' + filename_).c_str());
  }
}

}  // namespace mt
