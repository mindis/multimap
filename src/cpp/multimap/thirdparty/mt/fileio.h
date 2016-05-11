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

#ifndef MT_FILEIO_H_
#define MT_FILEIO_H_

#define _FILE_OFFSET_BITS 64
// Enables large file support (> 2 GiB) on 32-bit systems.

#include <sys/types.h>
#include <fcntl.h>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <boost/filesystem/path.hpp>             // NOLINT
#include <boost/iostreams/filtering_stream.hpp>  // NOLINT
#include "mt/common.h"

namespace mt {

namespace fs = boost::filesystem;

// -----------------------------------------------------------------------------
// POSIX-style I/O
// -----------------------------------------------------------------------------

class AutoCloseFd {
 public:
  static const int NOFD;

  AutoCloseFd() = default;

  explicit AutoCloseFd(int fd);

  AutoCloseFd(AutoCloseFd&& other);  // NOLINT

  ~AutoCloseFd();

  AutoCloseFd& operator=(AutoCloseFd&& other);  // NOLINT

  explicit operator bool() const noexcept;

  int get() const { return fd_; }

  int release();

  void reset(int fd = NOFD);

 private:
  int fd_ = NOFD;
};

AutoCloseFd open(const fs::path& file_path, int flags);

AutoCloseFd open(const fs::path& file_path, int flags, mode_t mode);

AutoCloseFd creat(const fs::path& file_path, mode_t mode);

void readAll(int fd, void* buf, size_t count);

bool readAllMaybe(int fd, void* buf, size_t count);

void preadAll(int fd, void* buf, size_t count, uint64_t offset);

bool preadAllMaybe(int fd, void* buf, size_t count, uint64_t offset);

void writeAll(int fd, const void* buf, size_t count);

void pwriteAll(int fd, const void* buf, size_t count, uint64_t offset);

uint64_t lseek(int fd, int64_t offset, int whence);

uint64_t ltell(int fd);

void ftruncate(int fd, uint64_t length);

// -----------------------------------------------------------------------------
// C-style I/O
// -----------------------------------------------------------------------------

class AutoCloseFile
    : public std::unique_ptr<std::FILE, decltype(std::fclose)*> {
  typedef std::unique_ptr<std::FILE, decltype(std::fclose)*> Base;

 public:
  AutoCloseFile() : AutoCloseFile(nullptr) {}

  AutoCloseFile(std::FILE* stream) : Base(stream, std::fclose) {}
};

AutoCloseFile fopen(const fs::path& file_path, const char* mode);

byte fgetc(std::FILE* stream);

bool fgetcMaybe(std::FILE* stream, byte* octet);

void fputc(std::FILE* stream, byte octet);

void freadAll(std::FILE* stream, void* buf, size_t count);

bool freadAllMaybe(std::FILE* stream, void* buf, size_t count);

void fwriteAll(std::FILE* stream, const void* buf, size_t count);

void fseek(std::FILE* stream, int64_t offset, int whence);

uint64_t ftell(std::FILE* stream);

// -----------------------------------------------------------------------------
// C++ stream-based I/O
// -----------------------------------------------------------------------------

typedef std::unique_ptr<std::istream> InputStream;
typedef std::unique_ptr<std::ostream> OutputStream;
typedef std::unique_ptr<std::iostream> InputOutputStream;

static const std::ios_base::openmode in_out_trunc =
    std::ios_base::in | std::ios_base::out | std::ios_base::trunc;

InputOutputStream newFileStream(
    const fs::path& file_path,
    std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out);

InputStream newFileInputStream(
    const fs::path& file_path,
    std::ios_base::openmode mode = std::ios_base::in);

OutputStream newFileOutputStream(
    const fs::path& file_path,
    std::ios_base::openmode mode = std::ios_base::out);

InputStream newGzipFileInputStream(
    const fs::path& file_path,
    std::ios_base::openmode mode = std::ios_base::in);

OutputStream newGzipFileOutputStream(
    const fs::path& file_path,
    std::ios_base::openmode mode = std::ios_base::out);

void readAll(std::istream* stream, void* buf, size_t count);

bool readAllMaybe(std::istream* stream, void* buf, size_t count);

void writeAll(std::ostream* stream, const void* buf, size_t count);

// -----------------------------------------------------------------------------
// Other
// -----------------------------------------------------------------------------

class DirectoryLockGuard {
 public:
  static const std::string DEFAULT_FILENAME;

  explicit DirectoryLockGuard(const fs::path& directory);

  DirectoryLockGuard(const fs::path& directory, const std::string& file_name);

  DirectoryLockGuard(const DirectoryLockGuard&) = delete;
  DirectoryLockGuard& operator=(const DirectoryLockGuard&) = delete;

  ~DirectoryLockGuard();

  const fs::path& directory() const { return directory_; }

  const std::string& file_name() const { return file_name_; }

 private:
  fs::path directory_;
  std::string file_name_;
};

Bytes readAllBytes(const fs::path& file_path);

std::vector<std::string> readAllLines(const fs::path& file_path);

std::vector<fs::path> listFiles(const fs::path& directory,
                                bool ignore_hidden = true);

}  // namespace mt

#endif  // MT_FILEIO_H_
