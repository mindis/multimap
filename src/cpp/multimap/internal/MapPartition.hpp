// This file is part of Multimap.  http://multimap.io
//
// Copyright (C) 2015-2016  Martin Trenkmann
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

#ifndef MULTIMAP_INTERNAL_MAP_PARTITION_HPP_INCLUDED
#define MULTIMAP_INTERNAL_MAP_PARTITION_HPP_INCLUDED

#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <boost/filesystem/path.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "multimap/internal/Arena.hpp"
#include "multimap/internal/List.hpp"
#include "multimap/internal/Stats.hpp"
#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/Iterator.hpp"

namespace multimap {
namespace internal {

class MapPartition : private mt::Resource {
  static const char* ATTEMPT_TO_MODIFY_READ_ONLY_PARTITION;

 public:
  struct Limits {
    static uint32_t maxKeySize();
    static uint32_t maxValueSize();
  };

  struct Options {
    uint32_t block_size = 512;
    uint32_t buffer_size = mt::MiB(1);
    bool create_if_missing = false;
    bool readonly = false;
  };

  explicit MapPartition(const boost::filesystem::path& file_prefix);

  MapPartition(const boost::filesystem::path& file_prefix,
               const Options& options);

  ~MapPartition();

  void put(const Bytes& key, const Bytes& value) {
    mt::Check::isFalse(isReadOnly(), ATTEMPT_TO_MODIFY_READ_ONLY_PARTITION);
    getOrCreateUniqueList(key).add(value);
  }

  std::unique_ptr<Iterator> get(const Bytes& key) const {
    return std::unique_ptr<Iterator>(
        new SharedListIterator(getSharedList(key)));
  }

  bool removeKey(const Bytes& key) {
    mt::Check::isFalse(isReadOnly(), ATTEMPT_TO_MODIFY_READ_ONLY_PARTITION);
    bool removed = false;
    auto list = getUniqueList(key);
    if (list && !list.empty()) {
      stats_.num_values_total += list.head().num_values_total;
      removed = true;
      list.clear();
    }
    return removed;
  }

  template <typename Predicate>
  uint32_t removeKeys(Predicate predicate) {
    mt::Check::isFalse(isReadOnly(), ATTEMPT_TO_MODIFY_READ_ONLY_PARTITION);
    uint32_t num_removed = 0;
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    for (const auto& entry : map_) {
      if (predicate(entry.first)) {
        ExclusiveList list(entry.second.get(), store_.get(), &arena_);
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
  uint32_t removeValues(const Bytes& key, Predicate predicate) {
    return remove(key, predicate, false);
  }

  bool replaceValue(const Bytes& key, const Bytes& old_value,
                    const Bytes& new_value) {
    return replaceValue(key, [&old_value, &new_value](const Bytes& value) {
      return (value == old_value) ? new_value.toString() : std::string();
    });
  }

  template <typename Function>
  bool replaceValue(const Bytes& key, Function map) {
    return replace(key, map, true);
  }

  uint32_t replaceValues(const Bytes& key, const Bytes& old_value,
                         const Bytes& new_value) {
    return replaceValues(key, [&old_value, &new_value](const Bytes& value) {
      return (value == old_value) ? new_value.toString() : std::string();
    });
  }

  template <typename Function>
  uint32_t replaceValues(const Bytes& key, Function map) {
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
    while (iter->hasNext()) {
      process(iter->next());
    }
  }

  template <typename BinaryProcedure>
  void forEachEntry(BinaryProcedure process) const {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    store_->adviseAccessPattern(Store::AccessPattern::WILLNEED);
    for (const auto& entry : map_) {
      SharedList list(*entry.second, *store_);
      if (!list.empty()) {
        SharedListIterator iter(std::move(list));
        process(entry.first, &iter);
      }
    }
    store_->adviseAccessPattern(Store::AccessPattern::NORMAL);
  }

  Stats getStats() const;
  // Returns various statistics about the partition.
  // The data is collected upon request and triggers a full partition scan.

  bool isReadOnly() const { return store_->isReadOnly(); }

  uint32_t getBlockSize() const { return store_->getBlockSize(); }

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
    for (size_t i = 0; i != stats.num_keys_valid; ++i) {
      const auto entry = Entry::readFromStream(stream.get(), &arena);
      SharedListIterator iter(SharedList(List(entry.head()), store));
      process(entry.key(), &iter);
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

  SharedList getSharedList(const Bytes& key) const;

  ExclusiveList getUniqueList(const Bytes& key);

  ExclusiveList getOrCreateUniqueList(const Bytes& key);

  template <typename Predicate>
  uint32_t remove(const Bytes& key, Predicate predicate,
                  bool exit_after_first_success) {
    mt::Check::isFalse(isReadOnly(), ATTEMPT_TO_MODIFY_READ_ONLY_PARTITION);
    uint32_t num_removed = 0;
    ExclusiveListIterator iter(getUniqueList(key));
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
  uint32_t replace(const Bytes& key, Function map,
                   bool exit_after_first_success) {
    mt::Check::isFalse(isReadOnly(), ATTEMPT_TO_MODIFY_READ_ONLY_PARTITION);
    std::vector<std::string> replaced_values;
    if (auto list = getUniqueList(key)) {
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

#endif  // MULTIMAP_INTERNAL_MAP_PARTITION_HPP_INCLUDED
