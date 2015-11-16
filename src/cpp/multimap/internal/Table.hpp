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

#ifndef MULTIMAP_INTERNAL_TABLE_HPP_INCLUDED
#define MULTIMAP_INTERNAL_TABLE_HPP_INCLUDED

#include <functional>
#include <memory>
#include <unordered_map>
#include <boost/filesystem/path.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "multimap/internal/Arena.hpp"
#include "multimap/internal/List.hpp"
#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/Callables.hpp"

namespace multimap {
namespace internal {

class Table {
public:
  struct Limits {
    static std::size_t getMaxKeySize();
    static std::size_t getMaxValueSize();
  };

  struct Options {
    std::size_t block_size = 512;
    std::size_t buffer_size = mt::MiB(1);
    bool create_if_missing = false;
    bool error_if_exists = false;
    bool readonly = false;
  };

  struct Stats {
    std::uint64_t block_size = 0;
    std::uint64_t num_blocks = 0;
    std::uint64_t num_keys = 0;
    std::uint64_t num_values_added = 0;
    std::uint64_t num_values_removed = 0;
    std::uint64_t num_values_unowned = 0;
    std::uint64_t key_size_min = -1;
    std::uint64_t key_size_max = 0;
    std::uint64_t key_size_avg = 0;
    std::uint64_t list_size_min = -1;
    std::uint64_t list_size_max = 0;
    std::uint64_t list_size_avg = 0;

    Stats& combine(const Stats& other);

    static Stats combine(const Stats& a, const Stats& b);

    static Stats fromProperties(const mt::Properties& properties);

    mt::Properties toProperties() const;
  };

  static_assert(std::is_standard_layout<Stats>::value,
                "Table::Stats is no standard layout type");

  static_assert(mt::hasExpectedSize<Stats>(80, 96),
                "Table::Stats does not have expected size");
  // Use __attribute__((packed)) if 32- and 64-bit size differ.

  typedef std::function<void(const Bytes&, SharedList&&)> BinaryProcedure;

  Table(const boost::filesystem::path& filepath_prefix);

  Table(const boost::filesystem::path& filepath_prefix, const Options& options);

  ~Table();

  SharedList getShared(const Bytes& key) const;

  UniqueList getUnique(const Bytes& key);

  UniqueList getUniqueOrCreate(const Bytes& key);

  void forEachKey(Callables::Procedure action) const;

  void forEachEntry(BinaryProcedure action) const;

  std::size_t getBlockSize() const { return store_->getBlockSize(); }

  std::size_t getBufferSize() const { return store_->getBufferSize(); }

  bool isReadOnly() const { return store_->isReadOnly(); }

  Stats getStats() const;
  // Returns various statistics about the table.
  // The data is collected upon request and triggers a full table scan.

private:
  mutable boost::shared_mutex mutex_;
  std::unordered_map<Bytes, std::unique_ptr<List> > map_;
  std::unique_ptr<Store> store_;
  Arena arena_;
  Stats old_stats_;
  boost::filesystem::path prefix_;
};

} // namespace internal
} // namespace multimap

#endif // MULTIMAP_INTERNAL_TABLE_HPP_INCLUDED
