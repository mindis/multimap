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

#include "multimap/internal/Mph.hpp"
#include "multimap/internal/Stats.hpp"
#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/Iterator.hpp"
#include "multimap/Bytes.hpp"

namespace multimap {
namespace internal {

class MphTable : public mt::Resource {
  // This class is read-only and therefore lock-free.

 public:
  struct Limits {
    static uint32_t maxKeySize();
    static uint32_t maxValueSize();
  };

  struct Options {
    bool quiet = false;
    std::function<bool(const Bytes&, const Bytes&)> compare;
  };

  typedef internal::Stats Stats;

  explicit MphTable(const boost::filesystem::path& prefix);

  ~MphTable();

  static Stats build(const boost::filesystem::path& datafile,
                     const Options& options);

  std::unique_ptr<Iterator> get(const Bytes& key) const;

  bool contains(const Bytes& key) const { return get(key).get(); }

  template <typename Procedure>
  void forEachKey(Procedure process) const {
    // TODO
  }

  template <typename Procedure>
  void forEachValue(const Bytes& key, Procedure process) const {
    // TODO
  }

  template <typename BinaryProcedure>
  void forEachEntry(BinaryProcedure process) const {
    // TODO
  }

  Stats getStats() const;
  // Returns various statistics about the partition.
  // The data is collected upon request and triggers a full partition scan.

  // ---------------------------------------------------------------------------
  // Static methods
  // ---------------------------------------------------------------------------

  template <typename BinaryProcedure>
  static void forEachEntry(const boost::filesystem::path& prefix,
                           const Options& options, BinaryProcedure process) {
    // TODO
  }

  static std::string getNameOfMphfFile(const std::string& prefix);
  static std::string getNameOfDataFile(const std::string& prefix);
  static std::string getNameOfListsFile(const std::string& prefix);
  static std::string getNameOfStatsFile(const std::string& prefix);

 private:
  std::unique_ptr<Mph> mph_;
  std::vector<uint32_t> data_;  // TODO Try to mmap this data
  boost::filesystem::path prefix_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_MPH_TABLE_HPP_INCLUDED
