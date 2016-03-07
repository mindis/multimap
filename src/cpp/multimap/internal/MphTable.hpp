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
#include "multimap/internal/Mph.hpp"
#include "multimap/internal/Stats.hpp"
#include "multimap/Iterator.hpp"

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
    bool quiet = false;
    std::function<bool(const Range&, const Range&)> compare;
  };

  typedef internal::Stats Stats;

  explicit MphTable(const std::string& prefix);

  std::unique_ptr<Iterator> get(const Range& key) const;

  template <typename Procedure>
  void forEachKey(Procedure process) const {
    // TODO
  }

  template <typename Procedure>
  void forEachValue(const Range& key, Procedure process) const {
    // TODO
  }

  template <typename BinaryProcedure>
  void forEachEntry(BinaryProcedure process) const {
    // TODO
  }

  Stats getStats() const { return stats_; }

  // ---------------------------------------------------------------------------
  // Static methods
  // ---------------------------------------------------------------------------

  static Stats build(const std::string& prefix, const std::string& input,
                     const Options& options);

  static Stats stats(const std::string& prefix);

  template <typename BinaryProcedure>
  static void forEachEntry(const std::string& prefix, const Options& options,
                           BinaryProcedure process) {
    // TODO
  }

 private:
  const Mph mph_;
  const std::vector<uint32_t> table_;  // TODO Try to mmap this data
  const Range lists_;
  const Stats stats_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_MPH_TABLE_HPP_INCLUDED
