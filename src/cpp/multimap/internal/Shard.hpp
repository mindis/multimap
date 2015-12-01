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

#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <boost/filesystem/path.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "multimap/internal/Arena.hpp"
#include "multimap/internal/List.hpp"
#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/Callables.hpp"

namespace multimap {
namespace internal {

class Shard : mt::Resource {
public:
  struct Limits {
    static std::size_t getMaxKeySize();
    static std::size_t getMaxValueSize();
  };

  struct Options {
    std::size_t block_size = 512;
    std::size_t buffer_size = mt::MiB(1);
    bool readonly = false;
    bool quiet = false;
  };

  struct Stats {
    std::uint64_t block_size = 0;
    std::uint64_t num_blocks = 0;
    std::uint64_t num_keys = 0;
    std::uint64_t num_values_put = 0;
    std::uint64_t num_values_rmd = 0;
    std::uint64_t key_size_min = 0;
    std::uint64_t key_size_max = 0;
    std::uint64_t key_size_avg = 0;
    std::uint64_t list_size_min = 0;
    std::uint64_t list_size_max = 0;
    std::uint64_t list_size_avg = 0;
    std::uint64_t checksum = 0;

    static const std::vector<std::string>& names();

    static Stats readFromFile(const boost::filesystem::path& file);

    void writeToFile(const boost::filesystem::path& file) const;

    static Stats fromProperties(const mt::Properties& properties);

    mt::Properties toProperties() const;

    std::vector<std::uint64_t> toVector() const;

    static Stats total(const std::vector<Stats>& stats);

    static Stats max(const std::vector<Stats>& stats);
  };

  static_assert(std::is_standard_layout<Stats>::value,
                "Shard::Stats is no standard layout type");

  static_assert(mt::hasExpectedSize<Stats>(96, 96),
                "Shard::Stats does not have expected size");
  // Use __attribute__((packed)) if 32- and 64-bit sizes differ.

  typedef SharedListIterator ListIterator;

  typedef UniqueListIterator MutableListIterator;

  Shard(const boost::filesystem::path& file_prefix);

  Shard(const boost::filesystem::path& file_prefix, const Options& options);

  ~Shard();

  void put(const Bytes& key, const Bytes& value);

  ListIterator get(const Bytes& key) const;

  MutableListIterator getMutable(const Bytes& key);

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

  void forEachKey(Callables::Procedure action,
                  bool ignore_empty_lists = true) const;

  void forEachValue(const Bytes& key, Callables::Procedure action) const;

  void forEachValue(const Bytes& key, Callables::Predicate action) const;

  void forEachEntry(Callables::BinaryProcedure action,
                    bool ignore_empty_lists = true) const;

  Stats getStats() const;
  // Returns various statistics about the shard.
  // The data is collected upon request and triggers a full table scan.

  bool isReadOnly() const { return store_->isReadOnly(); }

  std::size_t getBlockSize() const { return store_->getBlockSize(); }

  static std::string getNameOfKeysFile(const std::string& prefix);
  static std::string getNameOfStatsFile(const std::string& prefix);
  static std::string getNameOfValuesFile(const std::string& prefix);

private:
  SharedList getShared(const Bytes& key) const;
  UniqueList getUnique(const Bytes& key);
  UniqueList getUniqueOrCreate(const Bytes& key);

  mutable boost::shared_mutex mutex_;
  std::unordered_map<Bytes, std::unique_ptr<List> > map_;
  std::unique_ptr<Store> store_;
  Arena arena_;
  Stats stats_;
  boost::filesystem::path prefix_;
};

} // namespace internal
} // namespace multimap

#endif // MULTIMAP_INTERNAL_SHARD_HPP_INCLUDED
