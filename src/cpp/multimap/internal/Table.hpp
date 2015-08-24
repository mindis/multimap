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

#ifndef MULTIMAP_INTERNAL_TABLE_HPP
#define MULTIMAP_INTERNAL_TABLE_HPP

#include <cstdio>
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

struct TableFile {
  typedef std::pair<Bytes, List::Head> Entry;

  static void CreateInitialFile(const boost::filesystem::path& path);

  static Entry ReadEntryFromFile(std::FILE* file, Arena* arena);

  static void WriteEntryToFile(const Entry& entry, std::FILE* file);
};

class Table {
 public:
  Table();

  ~Table();

  void InitFromFile(std::FILE* file);

  SharedListLock GetShared(const Bytes& key) const;

  UniqueListLock GetUnique(const Bytes& key) const;

  UniqueListLock GetUniqueOrCreate(const Bytes& key);

  void ForEachKey(Callables::Procedure procedure) const;

  // Thread-safe: yes.
  std::map<std::string, std::string> GetProperties() const;

  void FlushLists(double min_load_factor) const;

  void FlushAllLists() const;

  const std::FILE* get_backing_file() const;

  void set_backing_file(std::FILE* file);

  const Callbacks::CommitBlock& get_commit_block_callback() const;

  void set_commit_block_callback(const Callbacks::CommitBlock& callback);

  static constexpr std::size_t max_key_size() {
    return std::numeric_limits<std::uint16_t>::max();
  }

 private:
  struct KeyHash {
    std::size_t operator()(const Bytes& key) const {
      // TODO Add compiler flags depending on CPU extensions.
      // Write the version of the hashing library into the super block.
      return util::Hash(static_cast<const char*>(key.data()), key.size());
    }
  };

  // TODO Implement a managed version of Bytes to be used as key_type.
  typedef std::unordered_map<Bytes, std::unique_ptr<List>, KeyHash> Map;

  Map map_;
  Arena arena_;
  mutable boost::shared_mutex mutex_;
  Callbacks::CommitBlock commit_block_;
  std::FILE* backing_file_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_TABLE_HPP
