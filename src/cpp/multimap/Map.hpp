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
#include "multimap/callables.hpp"
#include "multimap/Options.hpp"

namespace multimap {

static const uint32_t MAJOR_VERSION = 0;
static const uint32_t MINOR_VERSION = 3;

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
  // Returns a read-only iterator to the list associated with key. If no such
  // mapping exists the list is considered to be empty. If the list is not
  // empty a reader lock will be acquired to synchronize concurrent access to
  // it. Thus, multiple threads can read the list at the same time. Once
  // acquired, the lock is automatically released when the lifetime of the
  // iterator ends and its destructor is called. If the list is currently
  // locked exclusively by a writer lock, see getMutable(), the method will
  // block until the lock is released.

  bool removeKey(const Bytes& key) { return getTable(key).removeKey(key); }
  // Removes all values associated with `key`. This method will block until a
  // writer lock can be acquired for the associated list.
  // Returns: true if the key was removed, false otherwise.

  template <typename Predicate>
  uint32_t removeKeys(Predicate predicate) {
    uint32_t num_removed = 0;
    for (const auto& table : tables_) {
      num_removed += table->removeKeys(predicate);
    }
    return num_removed;
  }
  // Removes all values that are associated with keys for which `predicate`
  // yields `true`. This method will block until writer locks can be acquired
  // for the associated lists.
  // Returns: the number of removed keys.

  template <typename Predicate>
  bool removeValue(const Bytes& key, Predicate predicate) {
    return getTable(key).removeValue(key, predicate);
  }
  // Deletes the first value in the list associated with key for which
  // predicate yields true. This method will block until a writer lock can be
  // acquired.
  // Returns: true if a value was deleted, false otherwise.

  template <typename Predicate>
  uint32_t removeValues(const Bytes& key, Predicate predicate) {
    return getTable(key).removeValues(key, predicate);
  }
  // Deletes all values in the list associated with key for which predicate
  // yields true. This method will block until a writer lock can be acquired.
  // Returns: the number of deleted values.

  bool replaceValue(const Bytes& key, const Bytes& old_value,
                    const Bytes& new_value) {
    return getTable(key).replaceValue(key, old_value, new_value);
  }
  // TODO Document this.

  template <typename Function>
  bool replaceValue(const Bytes& key, Function map) {
    return getTable(key).replaceValue(key, map);
  }
  // Replaces the first value in the list associated with key by the result of
  // invoking function. If the result of function is an empty string no
  // replacement is performed. The replacement does not happen in-place.
  // Instead, the old value is marked as deleted and the new value is appended
  // to the end of the list. Future releases will support in-place
  // replacements. This method will block until a writer lock can be acquired.
  // Returns: true if a value was replaced, false otherwise.

  template <typename Function>
  uint32_t replaceValues(const Bytes& key, Function map) {
    return getTable(key).replaceValues(key, map);
  }
  // Replaces all values in the list associated with key by the result of
  // invoking function. If the result of function is an empty string no
  // replacement is performed. A replacement does not happen in-place. Instead,
  // the old value is marked as deleted and the new value is appended to the
  // end of the list. Future releases will support in-place replacements. This
  // method will block until a writer lock can be acquired.
  // Returns: the number of replaced values.

  uint32_t replaceValues(const Bytes& key, const Bytes& old_value,
                         const Bytes& new_value) {
    return getTable(key).replaceValues(key, old_value, new_value);
  }

  template <typename Procedure>
  void forEachKey(Procedure process) const {
    for (const auto& table : tables_) {
      table->forEachKey(process);
    }
  }
  // Applies `process` to each key of the map whose associated list is not
  // empty. The keys are visited in random order. Although it is possible to
  // emulate a for-each-entry iteration by keeping a `this` pointer of the map
  // within `procedure` plus calling `Get(key)`, this operation should be
  // avoided, because it produces a sub-optimal access pattern to the external
  // data files. Moreover, calling `GetMutable(key)` in such a scenario will
  // cause a deadlock. Use `forEachEntry` instead, which brings the entries in
  // an optimal order before iterating them. During the time of execution the
  // entire map is locked for read-only operations.

  template <typename Procedure>
  void forEachValue(const Bytes& key, Procedure process) const {
    getTable(key).forEachValue(key, process);
  }
  // Applies `process` to each value in the list associated with key. This is a
  // shorthand for requesting a read-only iterator via Get(key) followed by an
  // application of procedure to each value obtained via
  // ConstListIter::GetValue(). This method will block until a reader lock for
  // the list in question can be acquired.

  template <typename BinaryProcedure>
  void forEachEntry(BinaryProcedure process) const {
    for (const auto& table : tables_) {
      table->forEachEntry(process);
    }
  }

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
  // Static methods
  // ---------------------------------------------------------------------------

  static std::vector<Stats> stats(const boost::filesystem::path& directory);

  static void importFromBase64(const boost::filesystem::path& directory,
                               const boost::filesystem::path& input);
  // Imports key-value pairs from a Base64-encoded text file denoted by input
  // into the map located in the directory denoted by directory.
  // Preconditions:
  //   * The content in file follows the format described in Import / Export.
  // Throws std::exception if:
  //   * directory does not exist.
  //   * directory does not contain a map.
  //   * the map in directory is locked.
  //   * file is not a regular file.

  static void importFromBase64(const boost::filesystem::path& directory,
                               const boost::filesystem::path& input,
                               const Options& options);
  // Same as Import(directory, file) but creates a new map with default block
  // size if the directory denoted by directory does not contain a map and
  // create_if_missing is true.
  // Preconditions:
  //   * see Import(directory, file)
  // Throws std::exception if:
  //   * see Import(directory, file)
  //   * block_size is not a power of two.

  static void exportToBase64(const boost::filesystem::path& directory,
                             const boost::filesystem::path& output);
  // Exports all key-value pairs from the map located in the directory denoted
  // by `directory` to a Base64-encoded text file denoted by `file`. If the file
  // already exists, its content will be overridden.
  //
  // Tip: If `directory` and `file` point to different devices things will go
  // faster. If `directory` points to an SSD things will go even faster, at
  // least factor 2.
  //
  // Postconditions:
  //   * The content in `file` follows the format described in Import / Export.
  // Throws `std::exception` if:
  //   * `directory` does not exist.
  //   * `directory` does not contain a map.
  //   * the map in directory is locked.
  //   * `file` cannot be created.

  static void exportToBase64(const boost::filesystem::path& directory,
                             const boost::filesystem::path& output,
                             const Options& options);

  static void optimize(const boost::filesystem::path& directory,
                       const boost::filesystem::path& output);
  // Optimizes the map located in the directory denoted by `directory`
  // performing the following tasks:
  //
  //   * Defragmentation. All blocks which belong to the same list are written
  //     sequentially to disk which improves locality and leads to better read
  //     performance.
  //   * Garbage collection. Values marked as deleted will be removed physically
  //     which reduces the size of the map and also improves locality.
  //
  // Throws `std::exception` if:
  //
  //   * `directory` is not a directory.
  //   * `directory` does not contain a map.
  //   * the map in `directory` is locked.
  //   * `directory` is not writable.

  static void optimize(const boost::filesystem::path& directory,
                       const boost::filesystem::path& output,
                       const Options& options);
  // Optimizes the map located in the directory denoted by `directory`
  // performing the following tasks:
  //
  //   * Defragmentation. All blocks which belong to the same list are written
  //     sequentially to disk which improves locality and leads to better read
  //     performance.
  //   * Garbage collection. Values marked as deleted will be removed physically
  //     which reduces the size of the map and also improves locality.
  //
  // Throws `std::exception` if:
  //
  //   * `directory` is not a directory.
  //   * `directory` does not contain a map.
  //   * the map in `directory` is locked.
  //   * `directory` is not writable.
  //   * `new_block_size` is not a power of two.

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

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_MAP_HPP_INCLUDED
