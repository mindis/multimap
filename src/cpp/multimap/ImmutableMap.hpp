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
// Documentation:  http://multimap.io/cppreference/#immutablemaphpp
// -----------------------------------------------------------------------------

#ifndef MULTIMAP_IMMUTABLE_MAP_HPP_INCLUDED
#define MULTIMAP_IMMUTABLE_MAP_HPP_INCLUDED

#include <memory>
#include <vector>
#include "multimap/internal/Locks.hpp"
#include "multimap/internal/MphTable.hpp"

namespace multimap {

class ImmutableMap {
  // This class is read-only and does not need external locking.

 public:
  struct Limits {
    static uint32_t maxKeySize();
    static uint32_t maxValueSize();
  };

  struct Options : public internal::MphTable::Options {
    uint64_t max_bucket_size = mt::GiB(1);
    uint32_t num_buckets = 23;
  };

  typedef internal::MphTable::Procedure Procedure;
  typedef internal::MphTable::BinaryProcedure BinaryProcedure;

  class Builder {
   public:
    Builder(const boost::filesystem::path& directory, const Options& options);

    void put(const Slice& key, const Slice& value);

    std::vector<Stats> build();

   private:
    struct Bucket {
      boost::filesystem::path filename;
      mt::AutoCloseFile stream;
      bool is_large = false;
      uint64_t size = 0;
    };

    const Options options_;
    std::vector<Bucket> buckets_;
    internal::DirectoryLock dlock_;
  };

  explicit ImmutableMap(const boost::filesystem::path& directory);

  std::unique_ptr<Iterator> get(const Slice& key) const;

  void forEachKey(Procedure process) const;

  void forEachValue(const Slice& key, Procedure process) const;

  void forEachEntry(BinaryProcedure process) const;

  std::vector<Stats> getStats() const;

  Stats getTotalStats() const;

  // ---------------------------------------------------------------------------
  // Static member functions
  // ---------------------------------------------------------------------------

  static std::vector<Stats> stats(const boost::filesystem::path& directory);

  static void buildFromBase64(const boost::filesystem::path& directory,
                              const boost::filesystem::path& source);

  static void buildFromBase64(const boost::filesystem::path& directory,
                              const boost::filesystem::path& source,
                              const Options& options);

  static void exportToBase64(const boost::filesystem::path& directory,
                             const boost::filesystem::path& target);

  static void exportToBase64(const boost::filesystem::path& directory,
                             const boost::filesystem::path& target,
                             const Options& options);

 private:
  std::vector<internal::MphTable> tables_;
  internal::DirectoryLock dlock_;
};

}  // namespace multimap

#endif  // MULTIMAP_IMMUTABLE_MAP_HPP_INCLUDED
