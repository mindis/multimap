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

#include "mt/fileio.h"

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
#include <boost/filesystem/operations.hpp>  // NOLINT
#include "mt/check.h"

namespace mt {

Bytes readAllBytes(const boost::filesystem::path& file_path) {
  struct stat stats;
  const int result = stat(file_path.c_str(), &stats);
  Check::isZero(result, "stat() failed for %s because of '%s'",
                file_path.c_str(), errnostr());
  Bytes bytes(stats.st_size);
  const AutoCloseFd fd = open(file_path.c_str(), O_RDONLY);
  readAll(fd.get(), bytes.data(), bytes.size());
  return bytes;
}

std::vector<std::string> readAllLines(
    const boost::filesystem::path& file_path) {
  const InputFileStream stream = newInputFileStream(file_path);

  std::string line;
  std::vector<std::string> lines;
  while (std::getline(*stream, line)) {
    lines.push_back(line);
  }
  return lines;
}

// TODO Use boost::filesystem
std::vector<std::string> listFileNames(const boost::filesystem::path& directory,
                                       bool ignore_hidden) {
  DIR* stream = opendir(directory.c_str());
  Check::notNull(stream, "opendir() failed for %s because of '%s'",
                 directory.c_str(), errnostr());

  struct dirent* entry;
  std::vector<std::string> file_names;
  while ((entry = readdir(stream)) != nullptr) {
    if (entry->d_type == DT_REG) {
      if (ignore_hidden && entry->d_name[0] == '.') continue;
      file_names.emplace_back(entry->d_name);
    }
  }

  const int result = closedir(stream);
  Check::isZero(result, "closedir() failed because of '%s'", errnostr());
  std::sort(file_names.begin(), file_names.end());
  return file_names;
}

std::vector<boost::filesystem::path> listFilePaths(
    const boost::filesystem::path& directory, bool ignore_hidden) {
  const auto file_names = listFileNames(directory, ignore_hidden);
  std::vector<boost::filesystem::path> file_paths(file_names.size());
  for (size_t i = 0; i != file_names.size(); i++) {
    file_paths[i] = directory / file_names[i];
  }
  return file_paths;
}

const std::string DirectoryLockGuard::DEFAULT_FILENAME = ".lock";

DirectoryLockGuard::DirectoryLockGuard(const boost::filesystem::path& directory)
    : DirectoryLockGuard(directory, DEFAULT_FILENAME) {}

DirectoryLockGuard::DirectoryLockGuard(const boost::filesystem::path& directory,
                                       const std::string& file_name)
    : directory_(directory), file_name_(file_name) {
  const boost::filesystem::path file_path = directory / file_name;
  struct stat file_stats;
  Check::isEqual(-1, ::stat(file_path.c_str(), &file_stats),
                 "Could not create %s because the file already exists",
                 file_path.c_str());
  std::ofstream stream(file_path.string());
  Check::isTrue(stream.is_open(),
                "Could not create lock file %s for unknown reason",
                file_path.c_str());
  stream << ::getpid();
}

DirectoryLockGuard::~DirectoryLockGuard() {
  if (!directory_.empty()) {
    boost::filesystem::remove(directory_ / file_name_);
  }
}

const int AutoCloseFd::NOFD = -1;

AutoCloseFd::AutoCloseFd(int fd) : fd_(fd) {}

AutoCloseFd::AutoCloseFd(AutoCloseFd&& other)  // NOLINT
    : fd_(other.release()) {}

AutoCloseFd::~AutoCloseFd() { reset(); }

AutoCloseFd& AutoCloseFd::operator=(AutoCloseFd&& other) {  // NOLINT
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

AutoCloseFd open(const boost::filesystem::path& file_path, int flags) {
  AutoCloseFd fd(::open(file_path.c_str(), flags));
  Check::notEqual(AutoCloseFd::NOFD, fd.get(),
                  "open() failed for %s because of '%s'", file_path.c_str(),
                  errnostr());
  return fd;
}

AutoCloseFd open(const boost::filesystem::path& file_path, int flags,
                 mode_t mode) {
  AutoCloseFd fd(::open(file_path.c_str(), flags, mode));
  Check::notEqual(AutoCloseFd::NOFD, fd.get(),
                  "open() failed for %s because of '%s'", file_path.c_str(),
                  errnostr());
  return fd;
}

AutoCloseFd creat(const boost::filesystem::path& file_path, mode_t mode) {
  AutoCloseFd fd(::creat(file_path.c_str(), mode));
  Check::notEqual(AutoCloseFd::NOFD, fd.get(),
                  "creat() failed for %s because of '%s'", file_path.c_str(),
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

AutoCloseFile fopen(const boost::filesystem::path& file_path,
                    const char* mode) {
  AutoCloseFile file(std::fopen(file_path.c_str(), mode));
  Check::notNull(file.get(), "fopen() failed for %s because of '%s'",
                 file_path.c_str(), errnostr());
  return file;
}

byte fgetc(std::FILE* stream) {
  const int result = std::fgetc(stream);
  Check::notEqual(EOF, result, "fgetc() failed");
  return result;
}

bool fgetcMaybe(std::FILE* stream, byte* b) {
  const int result = std::fgetc(stream);
  if (result == EOF) return false;
  *b = result;
  return true;
}

void fputc(std::FILE* stream, byte b) {
  const int result = std::fputc(b, stream);
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

FileStream newFileStream(const boost::filesystem::path& file_path,
                         std::ios_base::openmode mode) {
  FileStream stream(new std::fstream(file_path.string(), mode));
  Check::isTrue(stream->is_open(), "Could not open %s", file_path.c_str());
  return stream;
}

InputFileStream newInputFileStream(const boost::filesystem::path& file_path,
                                   std::ios_base::openmode mode) {
  InputFileStream stream(new std::ifstream(file_path.string(), mode));
  Check::isTrue(stream->is_open(), "Could not open %s", file_path.c_str());
  return stream;
}

OutputFileStream newOutputFileStream(const boost::filesystem::path& file_path,
                                     std::ios_base::openmode mode) {
  OutputFileStream stream(new std::ofstream(file_path.string(), mode));
  Check::isTrue(stream->is_open(), "Could not open %s", file_path.c_str());
  return stream;
}

void readAll(std::istream* stream, void* buf, size_t count) {
  stream->read(static_cast<char*>(buf), count);
  Check::isEqual(stream->gcount(), count, "std::istream::read() failed");
}

bool readAllMaybe(std::istream* stream, void* buf, size_t count) {
  stream->read(static_cast<char*>(buf), count);
  return static_cast<size_t>(stream->gcount()) == count;
}

void writeAll(std::ostream* stream, const void* buf, size_t count) {
  stream->write(static_cast<const char*>(buf), count);
  Check::isFalse(stream->bad(), "std::istream::write() failed");
}

}  // namespace mt
