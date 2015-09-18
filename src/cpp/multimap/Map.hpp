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

#ifndef MULTIMAP_INCLUDE_MAP_HPP
#define MULTIMAP_INCLUDE_MAP_HPP

#include <map>
#include <string>
#include <boost/filesystem/path.hpp>
#include "multimap/internal/AutoCloseFile.hpp"
#include "multimap/internal/BlockPool.hpp"
#include "multimap/internal/Callbacks.hpp"
#include "multimap/internal/BlockStore.hpp"
#include "multimap/internal/Table.hpp"
#include "multimap/internal/System.hpp"
#include "multimap/Callables.hpp"
#include "multimap/Iterator.hpp"
#include "multimap/Options.hpp"

namespace multimap {

static const std::size_t MAJOR_VERSION = 0;
static const std::size_t MINOR_VERSION = 3;

// This class implements a 1:n key-value store where each key is associated
// with a list of values.
class Map {
 public:
  typedef Iterator<false> ListIter;
  // An iterator type to iterate a mutable list. All operations declared in the
  // class template Iterator<bool> that can mutate the underlying list are
  // enabled.

  typedef Iterator<true> ConstListIter;
  // An iterator type to iterate a immutable list. All operations declared in
  // the class template Iterator<bool> that can mutate the underlying list are
  // disabled.

  Map();
  // Creates a default instance which is not associated with a physical map.

  ~Map();
  // If associated with a physical map the destructor flushes all data to disk
  // and ensures that the map is stored in consistent state.
  // Preconditions:
  //   * No list is in locked state, i.e. there is no iterator object pointing
  //     to an existing list.

  Map(const boost::filesystem::path& directory, const Options& options);
  // Creates a new instance and opens the map located in directory. If the map
  // does not exist and options.create_if_missing is set to true a new map will
  // be created.
  // Throws std::exception if:
  //   * directory does not exist.
  //   * directory does not contain a map and options.create_if_missing is
  //     false.
  //   * directory contains a map and options.error_if_exists is true.
  //   * options.block_size is not a power of two.

  void open(const boost::filesystem::path& directory, const Options& options);
  // Opens the map located in directory. If the map does not exist and
  // options.create_if_missing is set to true a new map will be created.
  // Throws std::exception if:
  //   * directory does not exist.
  //   * directory does not contain a map and options.create_if_missing is
  //     false.
  //   * directory contains a map and options.error_if_exists is true.
  //   * options.block_size is not a power of two.

  void put(const Bytes& key, const Bytes& value);
  // Appends value to the end of the list associated with key.
  // Throws std::exception if:
  //   * key.size() > max_key_size()
  //   * value.size() > max_value_size()

  ConstListIter get(const Bytes& key) const;
  // Returns a read-only iterator to the list associated with key. If no such
  // mapping exists the list is considered to be empty. If the list is not
  // empty a reader lock will be acquired to synchronize concurrent access to
  // it. Thus, multiple threads can read the list at the same time. Once
  // acquired, the lock is automatically released when the lifetime of the
  // iterator ends and its destructor is called. If the list is currently
  // locked exclusively by a writer lock, see getMutable(), the method will
  // block until the lock is released.

  ListIter getMutable(const Bytes& key);
  // Returns a read-write iterator to the list associated with key. If no such
  // mapping exists the list is considered to be empty. If the list is not
  // empty a writer lock will be acquired to synchronize concurrent access to
  // it. Only one thread can acquire a writer lock at a time, since it requires
  // exclusive access. Once acquired, the lock is automatically released when
  // the lifetime of the iterator ends and its destructor is called. If the
  // list is currently locked, either by a reader or writer lock, the method
  // will block until the lock is released.

  bool contains(const Bytes& key) const;
  // Returns true if the list associated with key contains at least one value,
  // returns false otherwise. If the key does not exist the list is considered
  // to be empty. If a non-empty list is currently locked, the method will
  // block until the lock is released.

  std::size_t remove(const Bytes& key);
  // Deletes all values for key by clearing the associated list. This method
  // will block until a writer lock can be acquired.
  // Returns: the number of deleted values.

  std::size_t removeAll(const Bytes& key, Callables::BytesPredicate predicate);
  // Deletes all values in the list associated with key for which predicate
  // yields true. This method will block until a writer lock can be acquired.
  // Returns: the number of deleted values.

  std::size_t removeAllEqual(const Bytes& key, const Bytes& value);
  // Deletes all values in the list associated with key which are equal to
  // value according to operator==(const Bytes&, const Bytes&). This method
  // will block until a writer lock can be acquired.
  // Returns: the number of deleted values.

  bool removeFirst(const Bytes& key, Callables::BytesPredicate predicate);
  // Deletes the first value in the list associated with key for which
  // predicate yields true. This method will block until a writer lock can be
  // acquired.
  // Returns: true if a value was deleted, false otherwise.

  bool removeFirstEqual(const Bytes& key, const Bytes& value);
  // Deletes the first value in the list associated with key which is equal to
  // value according to operator==(const Bytes&, const Bytes&). This method
  // will block until a writer lock can be acquired.
  // Returns: true if a value was deleted, false otherwise.

  std::size_t replaceAll(const Bytes& key, Callables::BytesFunction function);
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

  bool replaceFirst(const Bytes& key, Callables::BytesFunction function);
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

  void forEachEntry(Callables::EntryProcedure procedure) const;

  void forEachKey(Callables::BytesProcedure procedure) const;
  // Applies `procedure` to each key of the map whose associated list is not
  // empty. The keys are visited in random order. Although it is possible to
  // emulate a for-each-entry iteration by keeping a `this` pointer of the map
  // within `procedure` plus calling `Get(key)`, this operation should be
  // avoided, because it produces a sub-optimal access pattern to the external
  // data files. Moreover, calling `GetMutable(key)` in such a scenario will
  // cause a deadlock. Use `forEachEntry` instead, which brings the entries in
  // an optimal order before iterating them. During the time of execution the
  // entire map is locked for read-only operations.

  void forEachValue(const Bytes& key,
                    Callables::BytesProcedure procedure) const;
  // Applies procedure to each value in the list associated with key. This is a
  // shorthand for requesting a read-only iterator via Get(key) followed by an
  // application of procedure to each value obtained via
  // ConstListIter::GetValue(). This method will block until a reader lock for
  // the list in question can be acquired.

  void forEachValue(const Bytes& key,
                    Callables::BytesPredicate predicate) const;
  // Applies predicate to each value in the list associated with key until
  // predicate yields false. This is a shorthand for requesting a read-only
  // iterator via Get(key) followed by an application of predicate to each
  // value obtained via ConstListIter::GetValue() until predicate yields false.
  // This method will block until a reader lock for the list in question can be
  // acquired.

  std::map<std::string, std::string> getProperties() const;
  // Returns a list of properties which describe the state of the map similar
  // to those written to the multimap.properties file. This method will only
  // look at lists which are currently not locked to be non-blocking.
  // Therefore, the returned values will be an approximation. For the time of
  // execution the map is locked for read-only operations.

  std::size_t max_key_size() const;
  // Returns the maximum size of a key in bytes which may be put. Currently
  // this is 65536 bytes.

  std::size_t max_value_size() const;
  // Returns the maximum size of a value in bytes which may be put. Currently
  // this is options.block_size - 2 bytes.

  // LONG RUNNING OPERATIONS (require exclusive access)
  // ---------------------------------------------------------------------------

  static void importFromBase64(const boost::filesystem::path& directory,
                               const boost::filesystem::path& file);
  // Imports key-value pairs from a Base64-encoded text file denoted by file
  // into the map located in the directory denoted by directory.
  // Preconditions:
  //   * The content in file follows the format described in Import / Export.
  // Throws std::exception if:
  //   * directory does not exist.
  //   * directory does not contain a map.
  //   * the map in directory is locked.
  //   * file is not a regular file.

  static void importFromBase64(const boost::filesystem::path& directory,
                               const boost::filesystem::path& file,
                               bool create_if_missing);
  // Same as Import(directory, file) but creates a new map with default block
  // size if the directory denoted by directory does not contain a map and
  // create_if_missing is true.
  // Preconditions:
  //   * see Import(directory, file)
  // Throws std::exception if:
  //   * see Import(directory, file)

  static void importFromBase64(const boost::filesystem::path& directory,
                               const boost::filesystem::path& file,
                               bool create_if_missing, std::size_t block_size);
  // Same as Import(directory, file, create_if_missing) but sets the block size
  // of a newly created map to block_size.
  // Preconditions:
  //   * see Import(directory, file, create_if_missing)
  // Throws std::exception if:
  //   * see Import(directory, file, create_if_missing)
  //   * block_size is not a power of two.

 private:
  void initCallbacks();

  std::size_t remove(const Bytes& key, Callables::BytesPredicate predicate,
                     bool apply_to_all);

  std::size_t replace(const Bytes& key, Callables::BytesFunction function,
                      bool apply_to_all);

  void importFromBase64(const boost::filesystem::path& file);

  internal::System::DirectoryLockGuard directory_lock_guard_;
  internal::BlockStore block_store_;
  internal::BlockPool block_pool_;
  internal::Callbacks callbacks_;
  internal::Table table_;
  Options options_;
};

void optimize(const boost::filesystem::path& from,
              const boost::filesystem::path& to);
// Copies the map located in the directory denoted by `from` to the directory
// denoted by `to` performing the following optimizations:
//   * Defragmentation. All blocks which belong to the same list are written
//     sequentially to disk which improves locality and leads to better read
//     performance.
//   * Garbage collection. Values that are marked as deleted won't be copied
//     which reduces the size of the new map and also improves locality.
//
// Tip: If `from` and `to` point to different devices things will go faster. If
// `from` points to an SSD things will go even faster, at least factor 2.
//
// Throws std::exception if:
//   * from or to are not directories.
//   * from does not contain a map.
//   * the map in from is locked.
//   * to already contains a map.
//   * to is not writable.

void optimize(const boost::filesystem::path& from,
              const boost::filesystem::path& to, std::size_t new_block_size);
// Same as Optimize(from, to) but sets the block size of the new map to
// new_block_size.
// Throws std::exception if:
//   * see Optimize(from, to)
//   * new_block_size is not a power of two.

void optimize(const boost::filesystem::path& from,
              const boost::filesystem::path& to,
              Callables::BytesCompare compare);
// Same as Optimize(from, to) but sorts each list before writing using compare
// as the sorting criterion.
// Throws std::exception if:
//   * see Optimize(from, to)

void optimize(const boost::filesystem::path& from,
              const boost::filesystem::path& to,
              Callables::BytesCompare compare, std::size_t new_block_size);
// Same as Optimize(from, to, compare) but sets the block size of the new map
// to new_block_size.
// Throws std::exception if:
//   * see Optimize(from, to, compare)
//   * new_block_size is not a power of two.

void exportToBase64(const boost::filesystem::path& directory,
                    const boost::filesystem::path& file);
// Exports all key-value pairs from the map located in the directory denoted by
// `directory` to a Base64-encoded text file denoted by `file`. If the file
// already exists, its content will be overridden.
//
// Tip: If `directory` and `file` point to different devices things will go
// faster. If `directory` points to an SSD things will go even faster, at least
// factor 2.
//
// Postconditions:
//   * The content in `file` follows the format described in Import / Export.
// Throws `std::exception` if:
//   * `directory` does not exist.
//   * `directory` does not contain a map.
//   * the map in directory is locked.
//   * `file` cannot be created.

}  // namespace multimap

#endif  // MULTIMAP_INCLUDE_MAP_HPP
