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

namespace multimap {
namespace internal {

class Shard : mt::Resource {
 public:
  struct Limits {
    static std::size_t maxKeySize();
    static std::size_t maxValueSize();
  };

  struct Options {
    std::size_t block_size = 512;
    std::size_t buffer_size = mt::MiB(1);
    bool create_if_missing = false;
    bool readonly = false;
    bool quiet = false;
  };

  struct Stats {
    std::uint64_t block_size = 0;
    std::uint64_t num_blocks = 0;
    std::uint64_t num_keys_total = 0;
    std::uint64_t num_keys_valid = 0;
    std::uint64_t num_values_total = 0;
    std::uint64_t num_values_valid = 0;
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

  static_assert(mt::hasExpectedSize<Stats>(104, 104),
                "Shard::Stats does not have expected size");
  // sizeof(Stats) must be equal on 32- and 64-bit systems to be portable.

  typedef SharedListIterator ListIterator;

  typedef UniqueListIterator MutableListIterator;

  explicit Shard(const boost::filesystem::path& file_prefix);

  Shard(const boost::filesystem::path& file_prefix, const Options& options);

  ~Shard();

  void put(const Bytes& key, const Bytes& value) {
    getUniqueOrCreate(key).add(value);
  }

  ListIterator get(const Bytes& key) const {
    return ListIterator(getShared(key));
  }

  MutableListIterator getMutable(const Bytes& key) {
    return MutableListIterator(getUnique(key));
  }

  bool removeKey(const Bytes& key) {
    bool removed = false;
    auto list = getUnique(key);
    if (list && !list.empty()) {
      stats_.num_values_total += list.head().num_values_total;
      removed = true;
      list.clear();
    }
    return removed;
  }

  template <typename Predicate>
  std::size_t removeKeys(Predicate predicate) {
    std::size_t num_removed = 0;
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    for (const auto& entry : map_) {
      if (predicate(entry.first)) {
        UniqueList list(entry.second.get(), store_.get(), &arena_);
        if (!list.empty()) {
          stats_.num_values_total += list.head().num_values_total;
          ++num_removed;
          list.clear();
        }
      }
    }
    return num_removed;
  }

  template <typename Predicate>
  bool removeValue(const Bytes& key, Predicate predicate) {
    return remove(key, predicate, true);
  }

  template <typename Predicate>
  std::size_t removeValues(const Bytes& key, Predicate predicate) {
    return remove(key, predicate, false);
  }

  template <typename Function>
  bool replaceValue(const Bytes& key, Function map) {
    return replace(key, map, true);
  }

  template <typename Function>
  std::size_t replaceValues(const Bytes& key, Function map) {
    return replace(key, map, false);
  }

  template <typename Procedure>
  void forEachKey(Procedure process) const {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    for (const auto& entry : map_) {
      SharedList list(*entry.second, *store_);
      if (!list.empty()) {
        process(entry.first);
      }
    }
  }

  template <typename Procedure>
  void forEachValue(const Bytes& key, Procedure process) const {
    auto iter = get(key);
    while (iter.hasNext()) {
      process(iter.next());
    }
  }

  template <typename BinaryProcedure>
  void forEachEntry(BinaryProcedure process) const {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    store_->adviseAccessPattern(Store::AccessPattern::WILLNEED);
    for (const auto& entry : map_) {
      SharedList list(*entry.second, *store_);
      if (!list.empty()) {
        process(entry.first, ListIterator(std::move(list)));
      }
    }
    store_->adviseAccessPattern(Store::AccessPattern::NORMAL);
  }

  Stats getStats() const;
  // Returns various statistics about the shard.
  // The data is collected upon request and triggers a full table scan.

  bool isReadOnly() const { return store_->isReadOnly(); }

  std::size_t getBlockSize() const { return store_->getBlockSize(); }

  // ---------------------------------------------------------------------------
  // Static methods
  // ---------------------------------------------------------------------------

  template <typename BinaryProcedure>
  static void forEachEntry(const boost::filesystem::path& prefix,
                           BinaryProcedure process) {
    Arena arena;
    Store::Options store_options;
    store_options.readonly = true;
    Store store(getNameOfValuesFile(prefix.string()), store_options);
    store.adviseAccessPattern(Store::AccessPattern::WILLNEED);
    const auto stats = Stats::readFromFile(getNameOfStatsFile(prefix.string()));
    const auto stream = mt::fopen(getNameOfKeysFile(prefix.string()), "r");
    for (std::size_t i = 0; i != stats.num_keys_valid; ++i) {
      const auto entry = Entry::readFromStream(stream.get(), &arena);
      List list(entry.head());
      process(entry.key(), ListIterator(SharedList(list, store)));
    }
  }

  static std::string getNameOfKeysFile(const std::string& prefix);
  static std::string getNameOfStatsFile(const std::string& prefix);
  static std::string getNameOfValuesFile(const std::string& prefix);

 private:
  struct Entry : public std::pair<Bytes, List::Head> {
    typedef std::pair<Bytes, List::Head> Base;

    Entry(Bytes&& key, List::Head&& head) : Base(key, head) {}

    Entry(const Bytes& key, const List::Head& head) : Base(key, head) {}

    Bytes& key() { return first; }
    const Bytes& key() const { return first; }

    List::Head& head() { return second; }
    const List::Head& head() const { return second; }

    static Entry readFromStream(std::FILE* stream, Arena* arena);

    void writeToStream(std::FILE* stream) const;
  };

  SharedList getShared(const Bytes& key) const;
  UniqueList getUnique(const Bytes& key);
  UniqueList getUniqueOrCreate(const Bytes& key);

  template <typename Predicate>
  std::size_t remove(const Bytes& key, Predicate predicate,
                     bool exit_after_first_success) {
    std::size_t num_removed = 0;
    auto iter = getMutable(key);
    while (iter.hasNext()) {
      if (predicate(iter.next())) {
        iter.remove();
        ++num_removed;
        if (exit_after_first_success) {
          break;
        }
      }
    }
    return num_removed;
  }

  template <typename Function>
  std::size_t replace(const Bytes& key, Function map,
                      bool exit_after_first_success) {
    std::vector<std::string> replaced_values;
    if (auto list = getUnique(key)) {
      auto iter = list.iterator();
      while (iter.hasNext()) {
        auto replaced_value = map(iter.next());
        if (!replaced_value.empty()) {
          replaced_values.push_back(std::move(replaced_value));
          iter.remove();
          if (exit_after_first_success) {
            break;
          }
        }
      }
      for (const auto& value : replaced_values) {
        list.add(value);
      }
    }
    return replaced_values.size();
  }

  mutable boost::shared_mutex mutex_;
  std::unordered_map<Bytes, std::unique_ptr<List> > map_;
  std::unique_ptr<Store> store_;
  Arena arena_;
  Stats stats_;
  boost::filesystem::path prefix_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_SHARD_HPP_INCLUDED
