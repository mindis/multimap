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
#include <boost/filesystem/path.hpp>  // NOLINT
#include "mt/common.h"

namespace mt {

Bytes readAllBytes(const boost::filesystem::path& file_path);

std::vector<std::string> readAllLines(const boost::filesystem::path& file_path);

std::vector<std::string> listFileNames(const boost::filesystem::path& directory,
                                       bool ignore_hidden = true);

std::vector<boost::filesystem::path> listFilePaths(
    const boost::filesystem::path& directory, bool ignore_hidden = true);

class DirectoryLockGuard {
 public:
  static const std::string DEFAULT_FILENAME;

  explicit DirectoryLockGuard(const boost::filesystem::path& directory);

  DirectoryLockGuard(const boost::filesystem::path& directory,
                     const std::string& file_name);

  DirectoryLockGuard(const DirectoryLockGuard&) = delete;
  DirectoryLockGuard& operator=(const DirectoryLockGuard&) = delete;

  ~DirectoryLockGuard();

  const boost::filesystem::path& directory() const { return directory_; }

  const std::string& file_name() const { return file_name_; }

 private:
  boost::filesystem::path directory_;
  std::string file_name_;
};

// -----------------------------------------------------------------------------
// POSIX-style I/O
// -----------------------------------------------------------------------------

class AutoCloseFd {
  // A RAII-style file descriptor owner.
  // The owned file descriptor, if any, is closed on destruction.

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

AutoCloseFd open(const boost::filesystem::path& file_path, int flags);
// Tries to open (or create) a file or thowns std::runtime_error on failure.
// Wraps open() from http://man7.org/linux/man-pages/man2/open.2.html

AutoCloseFd open(const boost::filesystem::path& file_path, int flags,
                 mode_t mode);
// Tries to open (or create) a file or thowns std::runtime_error on failure.
// Wraps open() from http://man7.org/linux/man-pages/man2/open.2.html

AutoCloseFd creat(const boost::filesystem::path& file_path, mode_t mode);
// Tries to create a file or thowns std::runtime_error on failure.
// Wraps creat() from http://man7.org/linux/man-pages/man2/open.2.html

void readAll(int fd, void* buf, size_t count);
// Tries to read exactly count bytes from a file descriptor or throws
// std::runtime_error if less or even no bytes could be read.
// Wraps read() from http://man7.org/linux/man-pages/man2/read.2.html

bool readAllMaybe(int fd, void* buf, size_t count);
// Tries to read exactly count bytes from a file descriptor. Returns true on
// success, false if less or even no bytes could be read.
// Wraps read() from http://man7.org/linux/man-pages/man2/read.2.html

void preadAll(int fd, void* buf, size_t count, uint64_t offset);
// Tries to read exactly count bytes from a file descriptor at a given offset
// or throws std::runtime_error if less or even no bytes could be read.
// Wraps pread() from http://man7.org/linux/man-pages/man2/pread.2.html

void writeAll(int fd, const void* buf, size_t count);
// Tries to write exactly count bytes to a file descriptor or throws
// std::runtime_error if less or even no bytes could be written.
// Wraps write() from http://man7.org/linux/man-pages/man2/write.2.html

void pwriteAll(int fd, const void* buf, size_t count, uint64_t offset);
// Tries to write exactly count bytes to a file descriptor at a given offset or
// throws std::runtime_error if less or even no bytes could be written.
// Wraps pwrite() from http://man7.org/linux/man-pages/man2/pwrite.2.html

uint64_t lseek(int fd, int64_t offset, int whence);
// Tries to reposition a file descriptor or throws std::runtime_error on error.
// Wraps lseek() from http://man7.org/linux/man-pages/man2/lseek.2.html

uint64_t tell(int fd);
// Tries to tell the current file offset or throws std::runtime_error on error.
// Wraps lseek() from before.

void ftruncate(int fd, uint64_t length);
// Tries to truncate a file to a specified length or throws std::runtime_error
// on failure.
// Wraps ftruncate() from http://man7.org/linux/man-pages/man2/ftruncate.2.html

// -----------------------------------------------------------------------------
// C-style I/O
// -----------------------------------------------------------------------------

class AutoCloseFile
    : public std::unique_ptr<std::FILE, decltype(std::fclose)*> {
  // A RAII-style file stream owner.
  // The owned file stream, if any, is closed on destruction.

  typedef std::unique_ptr<std::FILE, decltype(std::fclose)*> Base;

 public:
  AutoCloseFile() : AutoCloseFile(nullptr) {}

  AutoCloseFile(std::FILE* stream) : Base(stream, std::fclose) {}
};

AutoCloseFile fopen(const boost::filesystem::path& file_path, const char* mode);
// Tries to open (or create) a file or thowns std::runtime_error on failure.
// Wraps fopen() from http://man7.org/linux/man-pages/man3/fopen.3.html

byte fgetc(std::FILE* stream);
// Tries to read the next byte from a file stream or throws std::runtime_error
// on failure.
// Wraps fgetc() from http://man7.org/linux/man-pages/man3/fgetc.3.html

bool fgetcMaybe(std::FILE* stream, byte* b);
// Tries to read the next byte from a file stream. Returns true on success,
// false otherwise.
// Wraps fgetc() from http://man7.org/linux/man-pages/man3/fgetc.3.html

void fputc(std::FILE* stream, byte b);
// Tries to write one byte to a file stream or throws std::runtime_error on
// failure.
// Wraps fputc() from http://man7.org/linux/man-pages/man3/fputc.3.html

void freadAll(std::FILE* stream, void* buf, size_t count);
// Tries to read exactly count bytes from a file descriptor or throws
// std::runtime_error if less or even no bytes could be read.
// Wraps fread() from http://man7.org/linux/man-pages/man3/fread.3.html

bool freadAllMaybe(std::FILE* stream, void* buf, size_t count);
// Tries to read exactly count bytes from a file descriptor. Returns true on
// success, false if less or even no bytes could be read.
// Wraps fread() from http://man7.org/linux/man-pages/man3/fread.3.html

void fwriteAll(std::FILE* stream, const void* buf, size_t count);
// Tries to write exactly count bytes to a file stream or throws
// std::runtime_error if less or even no bytes could be written.
// Wraps fwrite() from http://man7.org/linux/man-pages/man3/fwrite.3.html

void fseek(std::FILE* stream, int64_t offset, int whence);
// Tries to reposition a file stream or throws std::runtime_error on failure.
// Wraps fseek() from http://man7.org/linux/man-pages/man3/fseek.3.html

uint64_t ftell(std::FILE* stream);
// Tries to tell the current file offset or throws std::runtime_error on error.
// Wraps ftell() from http://man7.org/linux/man-pages/man3/ftell.3.html

// -----------------------------------------------------------------------------
// C++ stream I/O
// -----------------------------------------------------------------------------

typedef std::unique_ptr<std::fstream> FileStream;
typedef std::unique_ptr<std::ifstream> InputFileStream;
typedef std::unique_ptr<std::ofstream> OutputFileStream;

static const std::ios_base::openmode in_out_trunc =
    std::ios_base::in | std::ios_base::out | std::ios_base::trunc;

FileStream newFileStream(const boost::filesystem::path& file_path,
                         std::ios_base::openmode mode = std::ios_base::in |
                                                        std::ios_base::out);

InputFileStream newInputFileStream(
    const boost::filesystem::path& file_path,
    std::ios_base::openmode mode = std::ios_base::in);

OutputFileStream newOutputFileStream(
    const boost::filesystem::path& file_path,
    std::ios_base::openmode mode = std::ios_base::out);

void readAll(std::istream* stream, void* buf, size_t count);

bool readAllMaybe(std::istream* stream, void* buf, size_t count);

void writeAll(std::ostream* stream, const void* buf, size_t count);

}  // namespace mt

#endif  // MT_FILEIO_H_
