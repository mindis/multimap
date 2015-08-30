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

#ifndef MULTIMAP_INTERNAL_SYSTEM_HPP
#define MULTIMAP_INTERNAL_SYSTEM_HPP

#include <sys/uio.h>
#include <cstdio>
#include <cstdint>
#include <ostream>
#include <string>
#include <utility>
#include <vector>
#include <boost/filesystem/path.hpp>

namespace multimap {
namespace internal {

struct System {
  static std::pair<boost::filesystem::path, int> Tempfile();

  // TODO Implement printf-like overloads.
  // Usage: System::Log() << "Your message here\n";
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

  class DirectoryLockGuard {
   public:
    static const std::string kDefaultFilename;

    DirectoryLockGuard();

    DirectoryLockGuard(const boost::filesystem::path& path);

    DirectoryLockGuard(const boost::filesystem::path& path,
                       const std::string filename);

    ~DirectoryLockGuard();

    void Lock(const boost::filesystem::path& path);

    void Lock(const boost::filesystem::path& path, const std::string filename);

    const boost::filesystem::path& path() const;

    const std::string& filename() const;

   private:
    boost::filesystem::path directory_;
    std::string filename_;
  };
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_SYSTEM_HPP
