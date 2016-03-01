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
#include "multimap/internal/MphTable.hpp"
#include "multimap/Version.hpp"

namespace multimap {

class ImmutableMap : public mt::Resource {
  // This class is read-only and therefore lock-free.

 public:
  struct Id {
    uint64_t num_tables = 0;
    uint64_t major_version = Version::MAJOR;
    uint64_t minor_version = Version::MINOR;

    static Id readFromDirectory(const boost::filesystem::path& directory);
    static Id readFromFile(const boost::filesystem::path& file);
    void writeToFile(const boost::filesystem::path& file) const;
  };

  static_assert(mt::hasExpectedSize<Id>(24, 24),
                "struct ImmutableMap::Id does not have expected size");

  struct Limits {
    static uint32_t maxKeySize();
    static uint32_t maxValueSize();
  };

  struct Options : public internal::MphTable::Options {
    uint64_t max_bucket_size = mt::GiB(1);
    uint32_t num_buckets = 23;
  };

  typedef internal::MphTable::Stats Stats;

  class Builder : public mt::Resource {
   public:
    Builder(const boost::filesystem::path& directory, const Options& options);

    ~Builder();

    void put(const Bytes& key, const Bytes& value);

    std::vector<Stats> build();

   private:
    struct Bucket {
      Bucket() = default;

      boost::filesystem::path filename;
      mt::AutoCloseFile stream;  // TODO Check if AutoCloseFd is equally fast
      bool is_large = false;
      uint64_t size = 0;
    };

    const Options options_;
    std::vector<Bucket> buckets_;
    mt::DirectoryLockGuard dlock_;
  };

  explicit ImmutableMap(const boost::filesystem::path& directory);

  ~ImmutableMap();

  std::unique_ptr<Iterator> get(const Bytes& key) const;

  bool contains(const Bytes& key) const { return getTable(key)->contains(key); }

  template <typename Procedure>
  void forEachKey(Procedure process) const {
    for (const auto& table : tables_) {
      table->forEachKey(process);
    }
  }

  template <typename Procedure>
  void forEachValue(const Bytes& key, Procedure process) const {
    getTable(key)->forEachValue(key, process);
  }

  template <typename BinaryProcedure>
  void forEachEntry(BinaryProcedure process) const {
    for (const auto& table : tables_) {
      table->forEachEntry(process);
    }
  }

  std::vector<Stats> getStats() const;

  Stats getTotalStats() const;

  // ---------------------------------------------------------------------------
  // Static member functions
  // ---------------------------------------------------------------------------

  static std::vector<Stats> stats(const boost::filesystem::path& directory);

  static void buildFromBase64(const boost::filesystem::path& directory,
                              const boost::filesystem::path& input);

  static void buildFromBase64(const boost::filesystem::path& directory,
                              const boost::filesystem::path& input,
                              const Options& options);

  static void exportToBase64(const boost::filesystem::path& directory,
                             const boost::filesystem::path& output);

  static void exportToBase64(const boost::filesystem::path& directory,
                             const boost::filesystem::path& output,
                             const Options& options);

 private:
  /*
  struct MphTableNode {
    std::unique_ptr<MphTableNode> left;
    std::unique_ptr<MphTableNode> right;
    std::unique_ptr<internal::MphTable> table;

    bool isLeaf() const { return !left && !right; }

    const internal::MphTable* get(const Bytes& key) const {
      const auto hash = mt::fnv1aHash(key.data(), key.size());
      return get(hash, 0);  // Check correctness of passing 0.
    }

    template <typename Procedure>
    // Requires: void operator()(const internal::MphTable&)
    void forEachLeaf(Procedure process) {
      if (isLeaf()) {
        process(*table);
      } else {
        left->forEachLeaf(process);
        right->forEachLeaf(process);
      }
    }

   private:
    const internal::MphTable* get(size_t hash, size_t depth) const;
  };
  */

  const internal::MphTable* getTable(const Bytes& key) const {
    return tables_[std::hash<Bytes>()(key) % tables_.size()].get();
  }

  std::vector<std::unique_ptr<internal::MphTable>> tables_;
  mt::DirectoryLockGuard dlock_;
};

}  // namespace multimap

#endif  // MULTIMAP_IMMUTABLE_MAP_HPP_INCLUDED
