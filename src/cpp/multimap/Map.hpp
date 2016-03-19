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

// -----------------------------------------------------------------------------
// Documentation:  https://multimap.io/cppreference/#maphpp
// -----------------------------------------------------------------------------

#ifndef MULTIMAP_MAP_HPP_INCLUDED
#define MULTIMAP_MAP_HPP_INCLUDED

#include "multimap/internal/Locks.hpp"
#include "multimap/internal/Partition.hpp"

namespace multimap {

class Map {
 public:
  // ---------------------------------------------------------------------------
  // Member types
  // ---------------------------------------------------------------------------

  struct Limits {
    static size_t maxKeySize();
    static size_t maxValueSize();

    Limits() = delete;
  };

  // ---------------------------------------------------------------------------
  // Member functions
  // ---------------------------------------------------------------------------

  explicit Map(const boost::filesystem::path& directory);

  Map(const boost::filesystem::path& directory, const Options& options);

  void put(const Slice& key, const Slice& value);

  template <typename InputIter>
  void put(const Slice& key, InputIter begin, InputIter end) {
    getPartition(key)->put(key, begin, end);
  }

  std::unique_ptr<Iterator> get(const Slice& key) const;

  size_t remove(const Slice& key);

  bool removeFirstEqual(const Slice& key, const Slice& value);

  size_t removeAllEqual(const Slice& key, const Slice& value);

  bool removeFirstMatch(const Slice& key, Predicate predicate);

  size_t removeFirstMatch(Predicate predicate);

  size_t removeAllMatches(const Slice& key, Predicate predicate);

  std::pair<size_t, size_t> removeAllMatches(Predicate predicate);

  bool replaceFirstEqual(const Slice& key, const Slice& old_value,
                         const Slice& new_value);

  size_t replaceAllEqual(const Slice& key, const Slice& old_value,
                         const Slice& new_value);

  bool replaceFirstMatch(const Slice& key, Function map);

  size_t replaceAllMatches(const Slice& key, Function map);

  size_t replaceAllMatches(const Slice& key, Function2 map);

  void forEachKey(Procedure process) const;

  void forEachValue(const Slice& key, Procedure process) const;

  void forEachEntry(BinaryProcedure process) const;

  std::vector<Stats> getStats() const;

  Stats getTotalStats() const;

  // ---------------------------------------------------------------------------
  // Static member functions
  // ---------------------------------------------------------------------------

  static std::vector<Stats> stats(const boost::filesystem::path& directory);

  static void importFromBase64(const boost::filesystem::path& directory,
                               const boost::filesystem::path& source);

  static void importFromBase64(const boost::filesystem::path& directory,
                               const boost::filesystem::path& source,
                               const Options& options);

  static void exportToBase64(const boost::filesystem::path& directory,
                             const boost::filesystem::path& target);

  static void exportToBase64(const boost::filesystem::path& directory,
                             const boost::filesystem::path& target,
                             const Options& options);

  static void optimize(const boost::filesystem::path& directory,
                       const boost::filesystem::path& target);

  static void optimize(const boost::filesystem::path& directory,
                       const boost::filesystem::path& target,
                       const Options& options);

 private:
  internal::Partition* getPartition(const Slice& key);

  const internal::Partition* getPartition(const Slice& key) const;

  std::vector<std::unique_ptr<internal::Partition> > partitions_;
  internal::DirectoryLock dlock_;
};

}  // namespace multimap

#endif  // MULTIMAP_MAP_HPP_INCLUDED
