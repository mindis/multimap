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

#ifndef MULTIMAP_INCLUDE_INTERNAL_SHARD_HPP
#define MULTIMAP_INCLUDE_INTERNAL_SHARD_HPP

#include <functional>
#include <boost/filesystem/path.hpp>
#include "multimap/internal/thirdparty/mt.hpp"
#include "multimap/internal/Arena.hpp"
#include "multimap/internal/Store.hpp"
#include "multimap/internal/Iterator.hpp"
#include "multimap/internal/Table.hpp"
#include "multimap/Bytes.hpp"

namespace multimap {
namespace internal {

class Shard {
 public:
  struct Stats {
    Store::Stats store;
    Table::Stats table;

    Stats& summarize(const Stats& other);

    static Stats summarize(const Stats& a, const Stats& b);

    static Stats fromProperties(const mt::Properties& properties);

    mt::Properties toProperties() const;
  };

  typedef Iterator<true> ListIterator;
  typedef Iterator<false> MutableListIterator;

  typedef List::BytesPredicate BytesPredicate;
  typedef List::BytesProcedure BytesProcedure;
  typedef std::function<std::string(const Bytes&)> BytesFunction;
  typedef std::function<bool(const Bytes&, const Bytes&)> BytesCompare;
  typedef std::function<void(const Bytes&, ListIterator&&)> EntryProcedure;

  Shard() = default;

  ~Shard();

  Shard(const boost::filesystem::path& prefix);

  Shard(const boost::filesystem::path& prefix, std::size_t block_size);

  void open(const boost::filesystem::path& prefix);

  void open(const boost::filesystem::path& prefix, std::size_t block_size);

  bool isOpen() const;

  Stats close();

  void put(const Bytes& key, const Bytes& value);

  ListIterator get(const Bytes& key) const;

  MutableListIterator getMutable(const Bytes& key);

  bool contains(const Bytes& key) const;

  std::size_t remove(const Bytes& key);

  std::size_t removeAll(const Bytes& key, BytesPredicate predicate);

  std::size_t removeAllEqual(const Bytes& key, const Bytes& value);

  bool removeFirst(const Bytes& key, BytesPredicate predicate);

  bool removeFirstEqual(const Bytes& key, const Bytes& value);

  std::size_t replaceAll(const Bytes& key, BytesFunction function);

  std::size_t replaceAllEqual(const Bytes& key, const Bytes& old_value,
                              const Bytes& new_value);

  bool replaceFirst(const Bytes& key, BytesFunction function);

  bool replaceFirstEqual(const Bytes& key, const Bytes& old_value,
                         const Bytes& new_value);

  void forEachKey(BytesProcedure procedure) const;

  void forEachValue(const Bytes& key, BytesProcedure procedure) const;

  void forEachValue(const Bytes& key, BytesPredicate predicate) const;

  void forEachEntry(EntryProcedure procedure) const;

  std::size_t max_key_size() const;

  std::size_t max_value_size() const;

  Stats getStats() const;

 private:
  void initCallbacks();

  std::size_t remove(const Bytes& key, BytesPredicate predicate,
                     bool apply_to_all);

  std::size_t replace(const Bytes& key, BytesFunction function,
                      bool apply_to_all);

  Table table_;
  Store store_;
  Arena arena_;
  Callbacks callbacks_;
  boost::filesystem::path prefix_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INCLUDE_INTERNAL_SHARD_HPP
