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
#include "multimap/internal/Table.hpp"
#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/Options.hpp"
#include "multimap/version.hpp"

namespace multimap {

class Map : mt::Resource {
public:
  struct Id {
    uint64_t block_size = 0;
    uint64_t num_partitions = 0;
    uint64_t major_version = MAJOR_VERSION;
    uint64_t minor_version = MINOR_VERSION;

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

  typedef internal::Table::Stats Stats;

  typedef internal::Table::Iterator Iterator;

  explicit Map(const boost::filesystem::path& directory);

  Map(const boost::filesystem::path& directory, const Options& options);

  ~Map();

  void put(const Bytes& key, const Bytes& value) {
    getTable(key).put(key, value);
  }

  Iterator get(const Bytes& key) const { return getTable(key).get(key); }

  bool removeKey(const Bytes& key) { return getTable(key).removeKey(key); }

  template <typename Predicate> uint32_t removeKeys(Predicate predicate) {
    uint32_t num_removed = 0;
    for (const auto& table : tables_) {
      num_removed += table->removeKeys(predicate);
    }
    return num_removed;
  }

  template <typename Predicate>
  bool removeValue(const Bytes& key, Predicate predicate) {
    return getTable(key).removeValue(key, predicate);
  }

  template <typename Predicate>
  uint32_t removeValues(const Bytes& key, Predicate predicate) {
    return getTable(key).removeValues(key, predicate);
  }

  bool replaceValue(const Bytes& key, const Bytes& old_value,
                    const Bytes& new_value) {
    return getTable(key).replaceValue(key, old_value, new_value);
  }

  template <typename Function>
  bool replaceValue(const Bytes& key, Function map) {
    return getTable(key).replaceValue(key, map);
  }

  template <typename Function>
  uint32_t replaceValues(const Bytes& key, Function map) {
    return getTable(key).replaceValues(key, map);
  }

  uint32_t replaceValues(const Bytes& key, const Bytes& old_value,
                         const Bytes& new_value) {
    return getTable(key).replaceValues(key, old_value, new_value);
  }

  template <typename Procedure> void forEachKey(Procedure process) const {
    for (const auto& table : tables_) {
      table->forEachKey(process);
    }
  }

  template <typename Procedure>
  void forEachValue(const Bytes& key, Procedure process) const {
    getTable(key).forEachValue(key, process);
  }

  template <typename BinaryProcedure>
  void forEachEntry(BinaryProcedure process) const {
    for (const auto& table : tables_) {
      table->forEachEntry(process);
    }
  }

  size_t getNumPartitions() const { return tables_.size(); }

  std::vector<Stats> getStats() const {
    std::vector<Stats> stats;
    for (const auto& table : tables_) {
      stats.push_back(table->getStats());
    }
    return stats;
  }

  Stats getTotalStats() const { return Stats::total(getStats()); }

  bool isReadOnly() const { return tables_.front()->isReadOnly(); }

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
  internal::Table& getTable(const Bytes& key) {
    return *tables_[mt::fnv1aHash(key.data(), key.size()) % tables_.size()];
  }

  const internal::Table& getTable(const Bytes& key) const {
    return *tables_[mt::fnv1aHash(key.data(), key.size()) % tables_.size()];
  }

  std::vector<std::unique_ptr<internal::Table> > tables_;
  mt::DirectoryLockGuard lock_;
};

namespace internal {

const std::string getFilePrefix();
const std::string getNameOfIdFile();
const std::string getNameOfLockFile();
const std::string getTablePrefix(uint32_t index);
const std::string getNameOfKeysFile(uint32_t index);
const std::string getNameOfStatsFile(uint32_t index);
const std::string getNameOfValuesFile(uint32_t index);
void checkVersion(uint64_t major_version, uint64_t minor_version);

} // namespace internal
} // namespace multimap

#endif // MULTIMAP_MAP_HPP_INCLUDED
