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

#ifndef MULTIMAP_TABLE_HPP
#define MULTIMAP_TABLE_HPP

#include <functional>
#include <cstdint>
#include <istream>
#include <ostream>
#include <memory>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <boost/filesystem/path.hpp>
#include "multimap/internal/Block.hpp"
#include "multimap/internal/Check.hpp"
#include "multimap/internal/ListLock.hpp"
#include "multimap/internal/UintVector.hpp"
#include "multimap/thirdparty/farmhash.h"

namespace multimap {
namespace internal {

// TODO Change interface from std::ostream to std::FILE.
struct TableFile {
  typedef std::pair<Bytes, List::Head> Entry;

  static Entry ReadEntryFromStream(std::istream& stream);

  static void WriteEntryToStream(const Entry& entry, std::ostream& stream);

  static void WriteEntryToStream(const Bytes& key, const List::Head& head,
                                 std::ostream& stream);
};

class Table {
 public:
  typedef std::function<void(const Bytes&)> KeyProcedure;
  typedef std::function<void(const List&)> ListProcedure;

  ~Table();

  static std::unique_ptr<Table> Open(const boost::filesystem::path& filepath);

  static std::unique_ptr<Table> Open(
      const boost::filesystem::path& filepath,
      const Callbacks::CommitBlock& commit_block);

  static std::unique_ptr<Table> Create(const boost::filesystem::path& filepath);

  static std::unique_ptr<Table> Create(
      const boost::filesystem::path& filepath,
      const Callbacks::CommitBlock& commit_block);

  SharedListLock GetShared(const Bytes& key) const;

  UniqueListLock GetUnique(const Bytes& key);

  UniqueListLock GetUniqueOrCreate(const Bytes& key);

  bool Delete(const Bytes& key);

  bool Contains(const Bytes& key) const;

  // Applies procedure to every key.
  // The order of keys visited is undefined.
  void ForEachKey(KeyProcedure procedure) const;

  // Applies procedure to every list.
  // The order of lists visited is undefined.
  // Locked lists will not be visited at all.
  void ForEachList(ListProcedure procedure) const;

  // Thread-safe: yes.
  std::map<std::string, std::string> GetProperties() const;

  void FlushLists(double min_load_factor);

  void FlushAllLists();

  const Callbacks::CommitBlock& get_commit_block() const;

  void set_commit_block(const Callbacks::CommitBlock& commit_block);

 private:
  struct KeyHash {
    std::size_t operator()(const Bytes& key) const {
      // TODO Add compiler flags depending on CPU extensions.
      // Write the version of the hashing library into the super block.
      return util::Hash(static_cast<const char*>(key.data()), key.size());
    }
  };

  typedef std::unordered_map<Bytes, std::unique_ptr<List>, KeyHash> Map;

  static Map ReadMapFromFile(const boost::filesystem::path& from);

  // Preconditions:
  //  * All lists in map are flushed (list.block has no data)
  //  * No list is currently in use, i.e. is locked.
  static void WriteMapToFile(const Map& map, const boost::filesystem::path& to);

  Map map_;
  mutable std::mutex map_mutex_;
  boost::filesystem::path table_file_;
  Callbacks::CommitBlock commit_block_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_TABLE_HPP
