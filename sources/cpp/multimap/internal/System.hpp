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

#ifndef MULTIMAP_SYSTEM_HPP
#define MULTIMAP_SYSTEM_HPP

#include <sys/uio.h>
#include <cstdio>
#include <cstdint>
#include <ostream>
#include <string>
#include <utility>
#include <vector>
#include <boost/filesystem/path.hpp>
#include "multimap/Bytes.hpp"

namespace multimap {
namespace internal {

struct System {
  static std::pair<boost::filesystem::path, int> Tempfile();

  static bool LockDirectory(const boost::filesystem::path& directory);

  static bool UnlockDirectory(const boost::filesystem::path& directory);

  // TODO Overload for printf-like capabilities.
  static std::ostream& Log();

  static std::ostream& Log(std::ostream& stream);

  static std::ostream& Log(const std::string& prefix);

  static std::ostream& Log(const std::string& prefix, std::ostream& stream);

  static std::ostream& PrintTimestamp(std::ostream& stream);

  // C-style I/O wrapper.

  static void Close(std::FILE* stream);

  static std::uint64_t Offset(std::FILE* stream);

  static void Seek(std::FILE* stream, std::uint64_t offset);

  static void Read(std::FILE* stream, void* buf, std::size_t count);

  static void Write(std::FILE* stream, const void* buf, std::size_t count);

  // POSIX-style I/O wrapper.

  static int Open(const boost::filesystem::path& filepath);

  static int Create(const boost::filesystem::path& filepath);

  static void Close(int fd);

  static std::uint64_t Offset(int fd);

  static void Seek(int fd, std::uint64_t offset);

  static void Read(int fd, void* buf, std::size_t count);

  static void Read(int fd, void* buf, std::size_t count, std::uint64_t offset);

  static void Write(int fd, const void* buf, std::size_t count);

  static void Write(int fd, const void* buf, std::size_t count,
                    std::uint64_t offset);

  class Batch {
   public:
    static const std::size_t kMaxSize;

    bool TryAdd(const void* data, std::size_t size);

    void Add(const void* data, std::size_t size);

    std::uint64_t Write(int fd) const;

    std::size_t size() const;

    bool empty() const;

    bool full() const;

    void clear();

   private:
    std::vector<iovec> items_;
  };
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_SYSTEM_HPP
