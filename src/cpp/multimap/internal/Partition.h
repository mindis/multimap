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

#ifndef MULTIMAP_INTERNAL_PARTITION_H_
#define MULTIMAP_INTERNAL_PARTITION_H_

#include <unordered_map>
#include <boost/filesystem/path.hpp>
#include "multimap/internal/List.h"
#include "multimap/Stats.h"

namespace multimap {
namespace internal {

class Partition {
 public:
  struct Limits {
    static size_t maxKeySize();
    static size_t maxValueSize();

    Limits() = delete;
  };

  explicit Partition(const boost::filesystem::path& prefix);

  Partition(const boost::filesystem::path& prefix, const Options& options);

  ~Partition();

  void put(const Slice& key, const Slice& value);

  template <typename InputIter>
  void put(const Slice& key, InputIter begin, InputIter end) {
    getListOrCreate(key)->append(begin, end, &store_, &arena_);
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

  Stats getStats() const;
  // Returns various statistics about the partition.
  // The data is collected upon request and triggers a full partition scan.

  static void forEachEntry(const boost::filesystem::path& prefix,
                           BinaryProcedure process);

  static Stats stats(const boost::filesystem::path& prefix);

 private:
  List* getList(const Slice& key) const;

  List* getListOrCreate(const Slice& key);

  mutable boost::shared_mutex mutex_;
  std::unordered_map<Slice, std::unique_ptr<List>> map_;
  Store store_;
  Arena arena_;
  Stats stats_;
  boost::filesystem::path prefix_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_PARTITION_H_
