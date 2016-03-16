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

#include <functional>
#include <boost/filesystem/path.hpp>
#include "multimap/internal/Mph.hpp"
#include "multimap/Iterator.hpp"
#include "multimap/Stats.hpp"

namespace multimap {
namespace internal {

class MphTable {
  // This class is read-only and does not need external locking.

 public:
  struct Limits {
    static uint32_t maxKeySize();
    static uint32_t maxValueSize();
  };

  struct Options {
    bool verbose = true;
    std::function<bool(const Slice&, const Slice&)> compare;
  };

  typedef std::function<void(const Slice&)> Procedure;
  typedef std::function<void(const Slice&, Iterator*)> BinaryProcedure;

  explicit MphTable(const std::string& prefix);

  MphTable(MphTable&& other);

  MphTable& operator=(MphTable&& other);

  ~MphTable();

  std::unique_ptr<Iterator> get(const Slice& key) const;

  void forEachKey(Procedure process) const;

  void forEachValue(const Slice& key, Procedure process) const;

  void forEachEntry(BinaryProcedure process) const;

  Stats getStats() const { return stats_; }

  // ---------------------------------------------------------------------------
  // Static methods
  // ---------------------------------------------------------------------------

  static Stats build(const std::string& prefix,
                     const boost::filesystem::path& source,
                     const Options& options);

  static Stats stats(const std::string& prefix);

  static void forEachEntry(const std::string& prefix, BinaryProcedure process);

 private:
  Mph mph_;
  Slice table_;
  Slice lists_;
  Stats stats_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_MPH_TABLE_HPP_INCLUDED
