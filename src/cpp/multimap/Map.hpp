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
// Documentation:  http://multimap.io/cppreference/#maphpp
// -----------------------------------------------------------------------------

#ifndef MULTIMAP_MAP_HPP_INCLUDED
#define MULTIMAP_MAP_HPP_INCLUDED

#include <memory>
#include <vector>
#include "multimap/internal/Locks.hpp"
#include "multimap/internal/Partition.hpp"

namespace multimap {

class Map {
 public:
  // ---------------------------------------------------------------------------
  // Member types
  // ---------------------------------------------------------------------------

  struct Limits {
    static uint32_t maxKeySize();
    static uint32_t maxValueSize();
  };

  // ---------------------------------------------------------------------------
  // Member functions
  // ---------------------------------------------------------------------------

  explicit Map(const boost::filesystem::path& directory);

  Map(const boost::filesystem::path& directory, const Options& options);

  void put(const Slice& key, const Slice& value) {
    getPartition(key)->put(key, value);
  }

  template <typename InputIter>
  void put(const Slice& key, InputIter first, InputIter last) {
    getPartition(key)->put(key, first, last);
  }

  std::unique_ptr<Iterator> get(const Slice& key) const {
    return getPartition(key)->get(key);
  }

  uint32_t remove(const Slice& key) { return getPartition(key)->remove(key); }

  bool removeFirstEqual(const Slice& key, const Slice& value) {
    return getPartition(key)->removeFirstEqual(key, value);
  }

  uint32_t removeAllEqual(const Slice& key, const Slice& value) {
    return getPartition(key)->removeAllEqual(key, value);
  }

  template <typename Predicate>
  bool removeFirstMatch(const Slice& key, Predicate predicate) {
    return getPartition(key)->removeFirstMatch(key, predicate);
  }

  template <typename Predicate>
  uint32_t removeFirstMatch(Predicate predicate) {
    uint32_t num_values_removed = 0;
    for (const auto& partition : partitions_) {
      num_values_removed = partition->removeFirstMatch(predicate);
      if (num_values_removed != 0) break;
    }
    return num_values_removed;
  }

  template <typename Predicate>
  uint32_t removeAllMatches(const Slice& key, Predicate predicate) {
    return getPartition(key)->removeAllMatches(key, predicate);
  }

  template <typename Predicate>
  std::pair<uint32_t, uint64_t> removeAllMatches(Predicate predicate) {
    uint32_t num_keys_removed = 0;
    uint64_t num_values_removed = 0;
    for (const auto& partition : partitions_) {
      const auto result = partition->removeAllMatches(predicate);
      num_keys_removed += result.first;
      num_values_removed += result.second;
    }
    return {num_keys_removed, num_values_removed};
  }

  bool replaceFirstEqual(const Slice& key, const Slice& old_value,
                         const Slice& new_value) {
    return getPartition(key)->replaceFirstEqual(key, old_value, new_value);
  }

  uint32_t replaceAllEqual(const Slice& key, const Slice& old_value,
                           const Slice& new_value) {
    return getPartition(key)->replaceAllEqual(key, old_value, new_value);
  }

  template <typename Function>
  bool replaceFirstMatch(const Slice& key, Function map) {
    return getPartition(key)->replaceFirstMatch(key, map);
  }

  template <typename Function>
  uint32_t replaceAllMatches(const Slice& key, Function map) {
    return getPartition(key)->replaceAllMatches(key, map);
  }

  template <typename Procedure>
  void forEachKey(Procedure process) const {
    for (const auto& partition : partitions_) {
      partition->forEachKey(process);
    }
  }

  template <typename Procedure>
  void forEachValue(const Slice& key, Procedure process) const {
    getPartition(key)->forEachValue(key, process);
  }

  template <typename BinaryProcedure>
  void forEachEntry(BinaryProcedure process) const {
    for (const auto& partition : partitions_) {
      partition->forEachEntry(process);
    }
  }

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
  internal::Partition* getPartition(const Slice& key) {
    return partitions_[std::hash<Slice>()(key) % partitions_.size()].get();
  }

  const internal::Partition* getPartition(const Slice& key) const {
    return partitions_[std::hash<Slice>()(key) % partitions_.size()].get();
  }

  std::vector<std::unique_ptr<internal::Partition> > partitions_;
  internal::DirectoryLock dlock_;
};

}  // namespace multimap

#endif  // MULTIMAP_MAP_HPP_INCLUDED
