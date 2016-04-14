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

#ifndef MULTIMAP_INTERNAL_MPH_TABLE_HPP_INCLUDED
#define MULTIMAP_INTERNAL_MPH_TABLE_HPP_INCLUDED

#include <boost/filesystem/path.hpp>
#include "multimap/internal/Mph.hpp"
#include "multimap/thirdparty/mt/fileio.hpp"
#include "multimap/thirdparty/mt/memory.hpp"
#include "multimap/callables.hpp"
#include "multimap/Iterator.hpp"
#include "multimap/Options.hpp"
#include "multimap/Stats.hpp"

namespace multimap {
namespace internal {

class MphTable {
  // This class is read-only and does not need external locking.

 public:
  struct Limits {
    static size_t maxKeySize();
    static size_t maxValueSize();

    Limits() = delete;
  };

  class Builder {
   public:
    Builder(Builder&&) = default;
    Builder& operator=(Builder&&) = default;

    Builder(const std::string& prefix, const Options& options);

    ~Builder();

    void put(const Slice& key, const Slice& value);

    boost::filesystem::path getFilename() const;

    boost::filesystem::path releaseFile();

    size_t getFileSize() const;

    Stats build();

   private:
    mt::AutoCloseFile stream_;
    std::string prefix_;
    Options options_;
  };

  explicit MphTable(const std::string& prefix);

  std::unique_ptr<Iterator> get(const Slice& key) const;

  void forEachKey(Procedure process) const;

  void forEachValue(const Slice& key, Procedure process) const;

  void forEachEntry(BinaryProcedure process) const;

  Stats getStats() const { return stats_; }

  static Stats stats(const std::string& prefix);

  static void forEachEntry(const std::string& prefix, BinaryProcedure process);

 private:
  Mph mph_;
  mt::AutoUnmapMemory table_;
  mt::AutoUnmapMemory lists_;
  Stats stats_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_MPH_TABLE_HPP_INCLUDED
