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
#include "multimap/internal/thirdparty/farmhash.h"

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

    std::map<std::string, std::string> ToMap() const;

    std::map<std::string, std::string> ToMap(const std::string& prefix) const;
  };

  Table() = default;

  Table(const boost::filesystem::path& file);

  Table(const boost::filesystem::path& file, bool create_if_missing);

  ~Table();

  void Open(const boost::filesystem::path& file);

  void Open(const boost::filesystem::path& file, bool create_if_missing);

  void Close();

  SharedListLock GetShared(const Bytes& key) const;

  UniqueListLock GetUnique(const Bytes& key) const;

  UniqueListLock GetUniqueOrCreate(const Bytes& key);

  void ForEachKey(Callables::Procedure procedure) const;

  std::map<std::string, std::string> GetProperties() const;

  void FlushAllListsAndWaitIfLocked() const;

  void FlushAllListsOrThrowIfLocked() const;

  void FlushAllUnlockedLists() const;

  Stats GetStats() const;

  const Callbacks::CommitBlock& get_commit_block_callback() const;

  void set_commit_block_callback(const Callbacks::CommitBlock& callback);

  static constexpr std::size_t max_key_size() {
    return std::numeric_limits<std::uint16_t>::max();
  }

 private:
  struct KeyHash {
    std::size_t operator()(const Bytes& key) const {
      return util::Hash(key.data(), key.size());
    }
  };

  typedef std::unordered_map<Bytes, std::unique_ptr<List>, KeyHash> Map;

  Map map_;
  Arena arena_;
  boost::filesystem::path path_;
  mutable boost::shared_mutex mutex_;
  Callbacks::CommitBlock commit_block_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INCLUDE_INTERNAL_TABLE_HPP
