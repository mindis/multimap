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

#ifndef MULTIMAP_INCLUDE_INTERNAL_TABLE_HPP
#define MULTIMAP_INCLUDE_INTERNAL_TABLE_HPP

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <boost/filesystem/path.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "multimap/Callables.hpp"
#include "multimap/internal/Arena.hpp"
#include "multimap/internal/Callbacks.hpp"
#include "multimap/internal/ListLock.hpp"

namespace multimap {
namespace internal {

class Table {
 public:
  struct Stats {
    std::size_t num_keys = 0;
    std::size_t num_lists_empty = 0;
    std::size_t num_lists_locked = 0;
    std::size_t num_values_total = 0;
    std::size_t num_values_deleted = 0;
    std::size_t key_size_min = -1;
    std::size_t key_size_max = 0;
    std::size_t key_size_avg = 0;
    std::size_t list_size_min = -1;
    std::size_t list_size_max = 0;
    std::size_t list_size_avg = 0;

    void combine(const Stats& other);

    std::map<std::string, std::string> toMap() const;

    std::map<std::string, std::string> toMap(const std::string& prefix) const;
  };

  typedef std::function<void(const Bytes&, SharedListLock&&)> EntryProcedure;

  Table() = default;

  Table(const boost::filesystem::path& file);

  ~Table();

  void open(const boost::filesystem::path& file);

  void close();

  SharedListLock getShared(const Bytes& key) const;

  UniqueListLock getUnique(const Bytes& key) const;

  UniqueListLock getUniqueOrCreate(const Bytes& key);

  void forEachKey(Callables::BytesProcedure procedure) const;

  void forEachEntry(EntryProcedure procedure) const;

  std::map<std::string, std::string> getProperties() const;

  void flushAllListsAndWaitIfLocked() const;

  void flushAllListsOrThrowIfLocked() const;

  void flushAllUnlockedLists() const;

  Stats getStats() const;

  const Callbacks::CommitBlock& commit_block_callback() const;

  void set_commit_block_callback(const Callbacks::CommitBlock& callback);

  static constexpr std::size_t max_key_size() {
    return std::numeric_limits<std::uint16_t>::max();
  }

 private:
  typedef std::unordered_map<Bytes, std::unique_ptr<List>> Hashtable;

  Arena arena_;
  Hashtable map_;
  boost::filesystem::path path_;
  mutable boost::shared_mutex mutex_;
  Callbacks::CommitBlock commit_block_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INCLUDE_INTERNAL_TABLE_HPP
