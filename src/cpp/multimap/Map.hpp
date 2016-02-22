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
#include <boost/filesystem/path.hpp>
#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/internal/Partition.hpp"
#include "multimap/internal/Stats.hpp"
#include "multimap/Iterator.hpp"
#include "multimap/Options.hpp"
#include "multimap/Version.hpp"

namespace multimap {

class Map : public mt::Resource {
 public:
  struct Id {
    uint64_t block_size = 0;
    uint64_t num_partitions = 0;
    uint64_t major_version = Version::MAJOR;
    uint64_t minor_version = Version::MINOR;

    static Id readFromDirectory(const boost::filesystem::path& directory);
    static Id readFromFile(const boost::filesystem::path& file);
    void writeToFile(const boost::filesystem::path& file) const;
  };

  static_assert(mt::hasExpectedSize<Id>(32, 32),
                "struct Map::Id does not have expected size");

  struct Limits {
    static uint32_t maxKeySize();
    static uint32_t maxValueSize();
  };

  typedef internal::Stats Stats;

  explicit Map(const boost::filesystem::path& directory);

  Map(const boost::filesystem::path& directory, const Options& options);

  ~Map();

  void put(const Bytes& key, const Bytes& value) {
    getPartition(key)->put(key, value);
  }

  template <typename InputIter>
  void put(const Bytes& key, InputIter first, InputIter last) {
    getPartition(key)->put(key, first, last);
  }

  std::unique_ptr<Iterator> get(const Bytes& key) const {
    return getPartition(key)->get(key);
  }

  bool contains(const Bytes& key) const {
    return getPartition(key)->contains(key);
  }

  uint32_t remove(const Bytes& key) { return getPartition(key)->remove(key); }

  template <typename Predicate>
  uint32_t removeOne(Predicate predicate) {
    uint32_t num_values_removed = 0;
    for (const auto& partition : partitions_) {
      num_values_removed = partition->removeOne(predicate);
      if (num_values_removed != 0) break;
    }
    return num_values_removed;
  }

  template <typename Predicate>
  std::pair<uint32_t, uint64_t> removeAll(Predicate predicate) {
    uint32_t num_keys_removed = 0;
    uint64_t num_values_removed = 0;
    for (const auto& partition : partitions_) {
      const auto result = partition->removeAll(predicate);
      num_keys_removed += result.first;
      num_values_removed += result.second;
    }
    return {num_keys_removed, num_values_removed};
  }

  template <typename Predicate>
  bool removeOne(const Bytes& key, Predicate predicate) {
    return getPartition(key)->removeOne(key, predicate);
  }

  template <typename Predicate>
  uint32_t removeAll(const Bytes& key, Predicate predicate) {
    return getPartition(key)->removeAll(key, predicate);
  }

  bool replaceOne(const Bytes& key, const Bytes& old_value,
                  const Bytes& new_value) {
    return getPartition(key)->replaceOne(key, old_value, new_value);
  }

  template <typename Function>
  bool replaceOne(const Bytes& key, Function map) {
    return getPartition(key)->replaceOne(key, map);
  }

  uint32_t replaceAll(const Bytes& key, const Bytes& old_value,
                      const Bytes& new_value) {
    return getPartition(key)->replaceAll(key, old_value, new_value);
  }

  template <typename Function>
  uint32_t replaceAll(const Bytes& key, Function map) {
    return getPartition(key)->replaceAll(key, map);
  }

  template <typename Procedure>
  void forEachKey(Procedure process) const {
    for (const auto& partition : partitions_) {
      partition->forEachKey(process);
    }
  }

  template <typename Procedure>
  void forEachValue(const Bytes& key, Procedure process) const {
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

  bool isReadOnly() const;

  // ---------------------------------------------------------------------------
  // Static member functions
  // ---------------------------------------------------------------------------

  static std::vector<Stats> stats(const boost::filesystem::path& directory);

  static void importFromBase64(const boost::filesystem::path& directory,
                               const boost::filesystem::path& input);

  static void importFromBase64(const boost::filesystem::path& directory,
                               const boost::filesystem::path& input,
                               const Options& options);

  static void exportToBase64(const boost::filesystem::path& directory,
                             const boost::filesystem::path& output);

  static void exportToBase64(const boost::filesystem::path& directory,
                             const boost::filesystem::path& output,
                             const Options& options);

  static void optimize(const boost::filesystem::path& directory,
                       const boost::filesystem::path& output);

  static void optimize(const boost::filesystem::path& directory,
                       const boost::filesystem::path& output,
                       const Options& options);

 private:
  internal::Partition* getPartition(const Bytes& key) {
    const auto hash = mt::fnv1aHash(key.data(), key.size());
    return partitions_[hash % partitions_.size()].get();
  }

  const internal::Partition* getPartition(const Bytes& key) const {
    const auto hash = mt::fnv1aHash(key.data(), key.size());
    return partitions_[hash % partitions_.size()].get();
  }

  std::vector<std::unique_ptr<internal::Partition> > partitions_;
  mt::DirectoryLockGuard lock_;
};

namespace internal {

const std::string getFilePrefix();
const std::string getNameOfIdFile();
const std::string getNameOfLockFile();
const std::string getNameOfKeysFile(uint32_t index);
const std::string getNameOfStatsFile(uint32_t index);
const std::string getNameOfValuesFile(uint32_t index);
const std::string getPartitionPrefix(uint32_t index);
void checkVersion(uint64_t major_version, uint64_t minor_version);

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_MAP_HPP_INCLUDED
