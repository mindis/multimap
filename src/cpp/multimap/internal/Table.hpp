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

#include <functional>
#include <memory>
#include <unordered_map>
#include <boost/filesystem/path.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "multimap/internal/thirdparty/mt.hpp"
#include "multimap/internal/Arena.hpp"
#include "multimap/internal/ListLock.hpp"

namespace multimap {
namespace internal {

class Table {
 public:
  struct Stats {
    std::uint64_t num_keys = 0;
    std::uint64_t num_values_put = 0;
    std::uint64_t num_values_removed = 0;
    std::uint64_t num_values_unowned = 0;
    std::uint64_t key_size_min = -1;
    std::uint64_t key_size_max = 0;
    std::uint64_t key_size_avg = 0;
    std::uint64_t list_size_min = -1;
    std::uint64_t list_size_max = 0;
    std::uint64_t list_size_avg = 0;

    Stats& summarize(const Stats& other);

    static Stats summarize(const Stats& a, const Stats& b);

    static Stats fromProperties(const mt::Properties& properties);

    static Stats fromProperties(const mt::Properties& properties,
                                const std::string& prefix);

    mt::Properties toProperties() const;

    mt::Properties toProperties(const std::string& prefix) const;
  };

  static_assert(std::is_standard_layout<Stats>::value,
                "Table::Stats is no standard layout type");

  static_assert(mt::hasExpectedSize<Stats>(80, 80),
                "Table::Stats does not have expected size");
  // Use __attribute__((packed)) if 32- and 64-bit size differ.

  typedef std::function<void(const Bytes&)> BytesProcedure;

  typedef std::function<void(const Bytes&, SharedListLock&&)> EntryProcedure;

  typedef std::function<std::uint32_t(const Block&)> CommitBlock;

  Table() = default;

  ~Table();

  Table(const boost::filesystem::path& filepath);

  void open(const boost::filesystem::path& filepath);

  Stats close(CommitBlock commit_block_callback);

  bool isOpen() const;

  SharedListLock getShared(const Bytes& key) const;

  UniqueListLock getUnique(const Bytes& key) const;

  UniqueListLock getUniqueOrCreate(const Bytes& key);

  void forEachKey(BytesProcedure procedure) const;

  void forEachEntry(EntryProcedure procedure) const;

  Stats getStats() const;
  // Returns various statistics about the table.
  // The data is collected upon request and triggers a full table scan.

  static constexpr std::size_t max_key_size() {
    return std::numeric_limits<std::uint16_t>::max();
  }

 private:
  typedef std::unordered_map<Bytes, std::unique_ptr<List>> Hashtable;

  mutable boost::shared_mutex mutex_;
  Hashtable map_;
  Arena arena_;
  Stats old_stats_;
  boost::filesystem::path filepath_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INCLUDE_INTERNAL_TABLE_HPP
