// This file is part of the Multimap library.  http://multimap.io
//
// Copyright (C) 2015  Martin Trenkmann
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

#ifndef MULTIMAP_MAP_HPP_INCLUDED
#define MULTIMAP_MAP_HPP_INCLUDED

#include <memory>
#include <vector>
#include <boost/filesystem/path.hpp>
#include "multimap/internal/Shard.hpp"
#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/Callables.hpp"
#include "multimap/Options.hpp"

namespace multimap {

static const std::size_t MAJOR_VERSION = 0;
static const std::size_t MINOR_VERSION = 3;

class Map : mt::Resource {
  // Insanely fast 1:n key-value store.

public:
  struct Limits {
    // Provides static methods to request upper bounds.

    static std::size_t getMaxKeySize();
    static std::size_t getMaxValueSize();
  };

  typedef internal::Shard::Stats Stats;

  typedef internal::SharedListIterator ListIterator;
  // An iterator type to iterate an immutable list.

  typedef internal::UniqueListIterator MutableListIterator;
  // An iterator type to iterate a mutable list.

  Map(const boost::filesystem::path& directory, const Options& options);
  // Opens the map located in directory. If the map does not exist and
  // options.create_if_missing is set to true a new map will be created.
  // Throws std::exception if:
  //   * directory does not exist.
  //   * directory does not contain a map and options.create_if_missing is
  //     false.
  //   * directory contains a map and options.error_if_exists is true.
  //   * options.block_size is not a power of two.

  ~Map();
  // Flushes all data to disk and stores the map in persistent state.
  // Preconditions:
  //   * No list is in locked state, i.e. there is no iterator object pointing
  //     to an existing list.

  void put(const Bytes& key, const Bytes& value);
  // Appends value to the end of the list associated with key.
  // Throws std::exception if:
  //   * key.size() > max_key_size()
  //   * value.size() > max_value_size()

  ListIterator get(const Bytes& key) const;
  // Returns a read-only iterator to the list associated with key. If no such
  // mapping exists the list is considered to be empty. If the list is not
  // empty a reader lock will be acquired to synchronize concurrent access to
  // it. Thus, multiple threads can read the list at the same time. Once
  // acquired, the lock is automatically released when the lifetime of the
  // iterator ends and its destructor is called. If the list is currently
  // locked exclusively by a writer lock, see getMutable(), the method will
  // block until the lock is released.

  MutableListIterator getMutable(const Bytes& key);
  // Returns a read-write iterator to the list associated with key. If no such
  // mapping exists the list is considered to be empty. If the list is not
  // empty a writer lock will be acquired to synchronize concurrent access to
  // it. Only one thread can acquire a writer lock at a time, since it requires
  // exclusive access. Once acquired, the lock is automatically released when
  // the lifetime of the iterator ends and its destructor is called. If the
  // list is currently locked, either by a reader or writer lock, the method
  // will block until the lock is released.

  std::size_t remove(const Bytes& key);
  // Deletes all values for key by clearing the associated list. This method
  // will block until a writer lock can be acquired.
  // Returns: the number of deleted values.

  std::size_t removeAll(const Bytes& key, Callables::Predicate predicate);
  // Deletes all values in the list associated with key for which predicate
  // yields true. This method will block until a writer lock can be acquired.
  // Returns: the number of deleted values.

  std::size_t removeAllEqual(const Bytes& key, const Bytes& value);
  // Deletes all values in the list associated with key which are equal to
  // value according to operator==(const Bytes&, const Bytes&). This method
  // will block until a writer lock can be acquired.
  // Returns: the number of deleted values.

  bool removeFirst(const Bytes& key, Callables::Predicate predicate);
  // Deletes the first value in the list associated with key for which
  // predicate yields true. This method will block until a writer lock can be
  // acquired.
  // Returns: true if a value was deleted, false otherwise.

  bool removeFirstEqual(const Bytes& key, const Bytes& value);
  // Deletes the first value in the list associated with key which is equal to
  // value according to operator==(const Bytes&, const Bytes&). This method
  // will block until a writer lock can be acquired.
  // Returns: true if a value was deleted, false otherwise.

  std::size_t replaceAll(const Bytes& key, Callables::Function function);
  // Replaces all values in the list associated with key by the result of
  // invoking function. If the result of function is an empty string no
  // replacement is performed. A replacement does not happen in-place. Instead,
  // the old value is marked as deleted and the new value is appended to the
  // end of the list. Future releases will support in-place replacements. This
  // method will block until a writer lock can be acquired.
  // Returns: the number of replaced values.

  std::size_t replaceAllEqual(const Bytes& key, const Bytes& old_value,
                              const Bytes& new_value);
  // Replaces all values in the list associated with key which are equal to
  // old_value according to operator==(const Bytes&, const Bytes&) by
  // new_value. A replacement does not happen in-place. Instead, the old value
  // is marked as deleted and the new value is appended to the end of the list.
  // Future releases will support in-place replacements. This method will block
  // until a writer lock can be acquired.
  // Returns: the number of replaced values.

  bool replaceFirst(const Bytes& key, Callables::Function function);
  // Replaces the first value in the list associated with key by the result of
  // invoking function. If the result of function is an empty string no
  // replacement is performed. The replacement does not happen in-place.
  // Instead, the old value is marked as deleted and the new value is appended
  // to the end of the list. Future releases will support in-place
  // replacements. This method will block until a writer lock can be acquired.
  // Returns: true if a value was replaced, false otherwise.

  bool replaceFirstEqual(const Bytes& key, const Bytes& old_value,
                         const Bytes& new_value);
  // Replaces the first value in the list associated with key which is equal to
  // old_value according to operator==(const Bytes&, const Bytes&) by
  // new_value. The replacement does not happen in-place. Instead, the old
  // value is marked as deleted and the new value is appended to the end of the
  // list. Future releases will support in-place replacements. This method will
  // block until a writer lock can be acquired.
  // Returns: true if a value was replaced, false otherwise.

  void forEachKey(Callables::Procedure action) const;
  // Applies `procedure` to each key of the map whose associated list is not
  // empty. The keys are visited in random order. Although it is possible to
  // emulate a for-each-entry iteration by keeping a `this` pointer of the map
  // within `procedure` plus calling `Get(key)`, this operation should be
  // avoided, because it produces a sub-optimal access pattern to the external
  // data files. Moreover, calling `GetMutable(key)` in such a scenario will
  // cause a deadlock. Use `forEachEntry` instead, which brings the entries in
  // an optimal order before iterating them. During the time of execution the
  // entire map is locked for read-only operations.

  void forEachValue(const Bytes& key, Callables::Procedure action) const;
  // Applies procedure to each value in the list associated with key. This is a
  // shorthand for requesting a read-only iterator via Get(key) followed by an
  // application of procedure to each value obtained via
  // ConstListIter::GetValue(). This method will block until a reader lock for
  // the list in question can be acquired.

  void forEachValue(const Bytes& key, Callables::Predicate action) const;
  // Applies predicate to each value in the list associated with key until
  // predicate yields false. This is a shorthand for requesting a read-only
  // iterator via Get(key) followed by an application of predicate to each
  // value obtained via ConstListIter::GetValue() until predicate yields false.
  // This method will block until a reader lock for the list in question can be
  // acquired.

  void forEachEntry(Callables::BinaryProcedure action) const;
  // TODO Document this.

  std::vector<Stats> getStats() const;

  Stats getTotalStats() const;

  bool isReadOnly() const;

private:
  std::vector<std::unique_ptr<internal::Shard> > shards_;
  mt::DirectoryLockGuard lock_;
};

namespace internal {

struct Id {
  std::uint64_t num_shards = 0;
  std::uint64_t major_version = MAJOR_VERSION;
  std::uint64_t minor_version = MINOR_VERSION;

  static Id readFromFile(const boost::filesystem::path& file);
  void writeToFile(const boost::filesystem::path& file) const;
};

const std::string getFilePrefix();
const std::string getNameOfIdFile();
const std::string getNameOfLockFile();
const std::string getShardPrefix(std::size_t index);
const std::string getNameOfKeysFile(std::size_t index);
const std::string getNameOfStatsFile(std::size_t index);
const std::string getNameOfValuesFile(std::size_t index);
void checkVersion(std::uint64_t major_version, std::uint64_t minor_version);

} // namespace internal
} // namespace multimap

#endif // MULTIMAP_MAP_HPP_INCLUDED
