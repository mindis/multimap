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

#ifndef MULTIMAP_INTERNAL_SHARD_HPP_INCLUDED
#define MULTIMAP_INTERNAL_SHARD_HPP_INCLUDED

#include <functional>
#include <boost/filesystem/path.hpp>
#include "multimap/internal/Arena.hpp"
#include "multimap/internal/Iterator.hpp"
#include "multimap/internal/Store.hpp"
#include "multimap/internal/Table.hpp"
#include "multimap/thirdparty/mt.hpp"
#include "multimap/Callables.hpp"

namespace multimap {
namespace internal {

class Shard {
public:
  struct Limits {
    static std::size_t max_key_size();
    static std::size_t max_value_size();
  };

  struct Stats {
    Table::Stats table;
    Store::Stats store;

    Stats& combine(const Stats& other);

    static Stats combine(const Stats& a, const Stats& b);

    static Stats fromProperties(const mt::Properties& properties);

    mt::Properties toProperties() const;
  };

  Shard() = default;

  Shard(const boost::filesystem::path& prefix);

  Shard(const boost::filesystem::path& prefix, std::size_t block_size);

  ~Shard();

  Stats close();

  void put(const Bytes& key, const Bytes& value);

  SharedListIterator getShared(const Bytes& key) const;

  UniqueListIterator getUnique(const Bytes& key);

  bool contains(const Bytes& key) const;

  std::size_t remove(const Bytes& key);

  std::size_t removeAll(const Bytes& key, Callables::Predicate predicate);

  std::size_t removeAllEqual(const Bytes& key, const Bytes& value);

  bool removeFirst(const Bytes& key, Callables::Predicate predicate);

  bool removeFirstEqual(const Bytes& key, const Bytes& value);

  std::size_t replaceAll(const Bytes& key, Callables::Function function);

  std::size_t replaceAllEqual(const Bytes& key, const Bytes& old_value,
                              const Bytes& new_value);

  bool replaceFirst(const Bytes& key, Callables::Function function);

  bool replaceFirstEqual(const Bytes& key, const Bytes& old_value,
                         const Bytes& new_value);

  void forEachKey(Callables::Procedure action) const;

  void forEachValue(const Bytes& key, Callables::Procedure action) const;

  void forEachValue(const Bytes& key, Callables::Predicate action) const;

  void forEachEntry(Callables::Procedure2 action) const;

  Stats getStats() const;

private:
  bool isOpen() const;

  std::size_t remove(const Bytes& key, Callables::Predicate predicate,
                     bool exit_on_first_success);

  std::size_t replace(const Bytes& key, Callables::Function function,
                      bool exit_on_first_success);

  Arena arena_;
  Table table_;
  Store store_;
  boost::filesystem::path prefix_;
};

} // namespace internal
} // namespace multimap

#endif // MULTIMAP_INTERNAL_SHARD_HPP_INCLUDED
