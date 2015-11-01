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

#ifndef MULTIMAP_INTERNAL_SYSTEM_HPP_INCLUDED
#define MULTIMAP_INTERNAL_SYSTEM_HPP_INCLUDED

#include <cstdio>
#include <cstdint>
#include <ostream>
#include <string>
#include <utility>
#include <boost/filesystem/path.hpp>

namespace multimap {
namespace internal {

struct System {
  static std::pair<boost::filesystem::path, int> getTempfile();

  static std::ostream& log();
  // TODO Implement printf-like overloads.
  // Usage: System::log() << "Your message here\n";

  static std::ostream& log(std::ostream& stream);

  static std::ostream& log(const std::string& prefix);

  static std::ostream& log(const std::string& prefix, std::ostream& stream);

  static std::ostream& printTimestamp(std::ostream& stream);

  // C-style I/O wrapper.

  static void close(std::FILE* stream);

  static std::uint64_t offset(std::FILE* stream);

  static void seek(std::FILE* stream, std::int64_t offset, int origin);

  static void read(std::FILE* stream, void* buf, std::size_t count);

  static void write(std::FILE* stream, const void* buf, std::size_t count);

  class DirectoryLockGuard {
   public:
    static const std::string DEFAULT_FILENAME;

    DirectoryLockGuard();

    explicit DirectoryLockGuard(const boost::filesystem::path& path);

    DirectoryLockGuard(const boost::filesystem::path& path,
                       const std::string filename);

    DirectoryLockGuard(const DirectoryLockGuard&) = delete;
    DirectoryLockGuard& operator=(const DirectoryLockGuard&) = delete;

    DirectoryLockGuard(DirectoryLockGuard&& other);
    DirectoryLockGuard& operator=(DirectoryLockGuard&& other);

    ~DirectoryLockGuard();

    void lock(const boost::filesystem::path& path);

    void lock(const boost::filesystem::path& path, const std::string filename);

    const boost::filesystem::path& path() const;

    const std::string& filename() const;

   private:
    boost::filesystem::path directory_;
    std::string filename_;
  };
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_SYSTEM_HPP_INCLUDED
