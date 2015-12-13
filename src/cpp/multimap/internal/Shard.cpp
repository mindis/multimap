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

#include "multimap/internal/Shard.hpp"

#include <boost/filesystem/operations.hpp>
#include "multimap/internal/Base64.hpp"

namespace multimap {
namespace internal {

namespace {

struct Entry : public std::pair<Bytes, List::Head> {

  typedef std::pair<Bytes, List::Head> Base;

  Entry(Bytes&& key, List::Head&& head) : Base(key, head) {}

  Entry(const Bytes& key, const List::Head& head) : Base(key, head) {}

  Bytes& key() { return first; }
  const Bytes& key() const { return first; }

  List::Head& head() { return second; }
  const List::Head& head() const { return second; }

  static Entry readFromStream(std::FILE* stream, Arena* arena) {
    std::int32_t key_size;
    mt::fread(stream, &key_size, sizeof key_size);
    const auto key_data = arena->allocate(key_size);
    mt::fread(stream, key_data, key_size);
    auto head = List::Head::readFromStream(stream);
    return Entry(Bytes(key_data, key_size), std::move(head));
  }

  void writeToStream(std::FILE* stream) const {
    MT_REQUIRE_LE(key().size(), Shard::Limits::maxKeySize());
    const std::int32_t key_size = key().size();
    mt::fwrite(stream, &key_size, sizeof key_size);
    mt::fwrite(stream, key().data(), key().size());
    head().writeToStream(stream);
  }
};

std::uint64_t computeChecksum(const Shard::Stats& stats) {
  auto copy = stats;
  copy.checksum = 0;
  return mt::crc32(&copy, sizeof copy);
}

std::size_t removeValues(UniqueList&& list, Callables::Predicate predicate,
                         bool exit_on_first_success) {
  std::size_t num_removed = 0;
  auto iter = list.iterator();
  while (iter.hasNext()) {
    if (predicate(iter.next())) {
      iter.remove();
      ++num_removed;
      if (exit_on_first_success) {
        break;
      }
    }
  }
  return num_removed;
}

std::size_t replaceValues(UniqueList&& list, Callables::Function map,
                          bool exit_on_first_success) {
  std::vector<std::string> replaced_values;
  auto iter = list.iterator();
  while (iter.hasNext()) {
    auto replaced_value = map(iter.next());
    if (!replaced_value.empty()) {
      replaced_values.push_back(std::move(replaced_value));
      iter.remove();
      if (exit_on_first_success) {
        break;
      }
    }
  }
  for (const auto& value : replaced_values) {
    list.add(value);
  }
  return replaced_values.size();
}

} // namespace

std::size_t Shard::Limits::maxKeySize() { return Varint::Limits::MAX_N4; }

std::size_t Shard::Limits::maxValueSize() {
  return List::Limits::maxValueSize();
}

const std::vector<std::string>& Shard::Stats::names() {
  static std::vector<std::string> names = {
    "block_size",     "num_blocks",    "num_keys",     "num_values_put",
    "num_values_rmd", "key_size_min",  "key_size_max", "key_size_avg",
    "list_size_min",  "list_size_max", "list_size_avg"
  };
  return names;
}

Shard::Stats Shard::Stats::readFromFile(const boost::filesystem::path& file) {
  Stats stats;
  const auto stream = mt::fopen(file, "r");
  mt::fread(stream.get(), &stats, sizeof stats);
  mt::check(stats.checksum == computeChecksum(stats), "Sanity check failed");
  return stats;
}

void Shard::Stats::writeToFile(const boost::filesystem::path& file) const {
  auto copy = *this;
  copy.checksum = computeChecksum(*this);
  const auto stream = mt::fopen(file.c_str(), "w");
  mt::fwrite(stream.get(), &copy, sizeof copy);
}

Shard::Stats Shard::Stats::fromProperties(const mt::Properties& properties) {
  Stats stats;
  stats.block_size = std::stoul(properties.at("block_size"));
  stats.num_blocks = std::stoul(properties.at("num_blocks"));
  stats.num_keys = std::stoul(properties.at("num_keys"));
  stats.num_values_put = std::stoul(properties.at("num_values_put"));
  stats.num_values_rmd = std::stoul(properties.at("num_values_rmd"));
  stats.key_size_min = std::stoul(properties.at("key_size_min"));
  stats.key_size_max = std::stoul(properties.at("key_size_max"));
  stats.key_size_avg = std::stoul(properties.at("key_size_avg"));
  stats.list_size_min = std::stoul(properties.at("list_size_min"));
  stats.list_size_max = std::stoul(properties.at("list_size_max"));
  stats.list_size_avg = std::stoul(properties.at("list_size_avg"));
  // stats.checksum is left default initialized.
  return stats;
}

mt::Properties Shard::Stats::toProperties() const {
  mt::Properties properties;
  properties["block_size"] = std::to_string(block_size);
  properties["num_blocks"] = std::to_string(num_blocks);
  properties["num_keys"] = std::to_string(num_keys);
  properties["num_values_put"] = std::to_string(num_values_put);
  properties["num_values_rmd"] = std::to_string(num_values_rmd);
  properties["key_size_min"] = std::to_string(key_size_min);
  properties["key_size_max"] = std::to_string(key_size_max);
  properties["key_size_avg"] = std::to_string(key_size_avg);
  properties["list_size_min"] = std::to_string(list_size_min);
  properties["list_size_max"] = std::to_string(list_size_max);
  properties["list_size_avg"] = std::to_string(list_size_avg);
  // stats.checksum is not exposed.
  return properties;
}

std::vector<std::uint64_t> Shard::Stats::toVector() const {
  return { block_size,     num_blocks,    num_keys,     num_values_put,
           num_values_rmd, key_size_min,  key_size_max, key_size_avg,
           list_size_min,  list_size_max, list_size_avg };
}

Shard::Stats Shard::Stats::total(const std::vector<Stats>& stats) {
  Shard::Stats total;
  for (const auto& stat : stats) {
    if (total.block_size == 0) {
      total.block_size = stat.block_size;
    } else {
      MT_ASSERT_EQ(total.block_size, stat.block_size);
    }
    total.num_blocks += stat.num_blocks;
    total.num_keys += stat.num_keys;
    total.num_values_put += stat.num_values_put;
    total.num_values_rmd += stat.num_values_rmd;
    total.key_size_max = std::max(total.key_size_max, stat.key_size_max);
    if (stat.key_size_min) {
      total.key_size_min = total.key_size_min
                               ? mt::min(total.key_size_min, stat.key_size_min)
                               : stat.key_size_min;
    }
    total.list_size_max = std::max(total.list_size_max, stat.list_size_max);
    if (stat.list_size_min) {
      total.list_size_min =
          total.list_size_min ? mt::min(total.list_size_min, stat.list_size_min)
                              : stat.list_size_min;
    }
  }
  if (total.num_keys != 0) {
    double key_size_avg = 0;
    double list_size_avg = 0;
    for (const auto& stat : stats) {
      const auto weight = stat.num_keys / static_cast<double>(total.num_keys);
      key_size_avg += weight * stat.key_size_avg;
      list_size_avg += weight * stat.list_size_avg;
    }
    total.key_size_avg = key_size_avg;
    total.list_size_avg = list_size_avg;
  }
  return total;
}

Shard::Stats Shard::Stats::max(const std::vector<Stats>& stats) {
  Shard::Stats max;
  for (const auto& stat : stats) {
    max.block_size = std::max(max.block_size, stat.block_size);
    max.num_blocks = std::max(max.num_blocks, stat.num_blocks);
    max.num_keys = std::max(max.num_keys, stat.num_keys);
    max.num_values_put = std::max(max.num_values_put, stat.num_values_put);
    max.num_values_rmd = std::max(max.num_values_rmd, stat.num_values_rmd);
    max.key_size_avg = std::max(max.key_size_avg, stat.key_size_avg);
    max.key_size_max = std::max(max.key_size_max, stat.key_size_max);
    if (stat.key_size_min) {
      max.key_size_min = std::max(max.key_size_min, stat.key_size_min);
    }
    max.list_size_avg = std::max(max.list_size_avg, stat.list_size_avg);
    max.list_size_max = std::max(max.list_size_max, stat.list_size_max);
    if (stat.list_size_min) {
      max.list_size_min = std::max(max.list_size_min, stat.list_size_min);
    }
  }
  return max;
}

Shard::Shard(const boost::filesystem::path& file_prefix)
    : Shard(file_prefix, Options()) {}

Shard::Shard(const boost::filesystem::path& file_prefix, const Options& options)
    : prefix_(file_prefix) {
  const auto stats_file = getNameOfStatsFile(prefix_.string());
  if (boost::filesystem::is_regular_file(stats_file)) {
    stats_ = Stats::readFromFile(stats_file);
    const auto keys_file = getNameOfKeysFile(prefix_.string());
    const auto stream = mt::fopen(keys_file, "r");
    for (std::size_t i = 0; i != stats_.num_keys; ++i) {
      const auto entry = Entry::readFromStream(stream.get(), &arena_);
      map_[entry.key()].reset(new List(entry.head()));
      stats_.num_values_put -= entry.head().num_values_added;
      stats_.num_values_rmd -= entry.head().num_values_removed;
    }

    // Reset stats, but preserve number of values put and removed.
    Stats stats;
    stats.num_values_put = stats_.num_values_put;
    stats.num_values_rmd = stats_.num_values_rmd;
    stats_ = stats;
  }

  Store::Options store_options;
  store_options.block_size = options.block_size; // Ignored if already exists
  store_options.buffer_size = options.buffer_size;
  store_options.readonly = options.readonly;
  store_options.quiet = options.quiet;
  store_.reset(new Store(getNameOfValuesFile(prefix_.string()), store_options));
}

Shard::~Shard() {
  if (!prefix_.empty() && !isReadOnly()) {
    const auto keys_file = getNameOfKeysFile(prefix_.string());
    const auto old_keys_file = keys_file + ".old";
    if (boost::filesystem::is_regular_file(keys_file)) {
      boost::filesystem::rename(keys_file, old_keys_file);
    }

    const auto stream = mt::fopen(keys_file, "w");
    const auto store_stats = store_->getStats();
    stats_.block_size = store_stats.block_size;
    stats_.num_blocks = store_stats.num_blocks;
    for (const auto& entry : map_) {
      const auto& key = entry.first;
      const auto& list = entry.second.get();
      if (list->is_locked()) {
        const auto key_as_base64 = Base64::encode(key);
        mt::log() << "The list with the key " << key_as_base64
                  << " (Base64) was still locked when shutting down."
                  << " Recent updates of the list may be lost.\n";
      }
      // We do not skip or even throw if a list is still locked to prevent data
      // loss. However, this causes a race which could let the program crash...
      list->flush(store_.get());
      stats_.num_values_put += list->head().num_values_added;
      stats_.num_values_rmd += list->head().num_values_removed;
      if (!list->empty()) {
        ++stats_.num_keys;
        stats_.key_size_avg += key.size();
        stats_.key_size_max = mt::max(stats_.key_size_max, key.size());
        stats_.key_size_min = stats_.key_size_min
                                  ? mt::min(stats_.key_size_min, key.size())
                                  : key.size();
        stats_.list_size_avg += list->size();
        stats_.list_size_max = mt::max(stats_.list_size_max, list->size());
        stats_.list_size_min = stats_.list_size_min
                                   ? mt::min(stats_.list_size_min, list->size())
                                   : list->size();
        Entry(key, list->head()).writeToStream(stream.get());
      }
    }

    if (stats_.num_keys) {
      stats_.key_size_avg /= stats_.num_keys;
      stats_.list_size_avg /= stats_.num_keys;
    }
    stats_.writeToFile(getNameOfStatsFile(prefix_.string()));

    if (boost::filesystem::is_regular_file(old_keys_file)) {
      const auto status = boost::filesystem::remove(old_keys_file);
      MT_ASSERT_TRUE(status);
    }
  }
}

void Shard::put(const Bytes& key, const Bytes& value) {
  getUniqueOrCreate(key).add(value);
}

Shard::ListIterator Shard::get(const Bytes& key) const {
  return ListIterator(getShared(key));
}

Shard::MutableListIterator Shard::getMutable(const Bytes& key) {
  return MutableListIterator(getUnique(key));
}

std::size_t Shard::remove(const Bytes& key) {
  std::size_t num_removed = 0;
  if (auto list = getUnique(key)) {
    stats_.num_values_put += list.head().num_values_added;
    stats_.num_values_rmd += list.head().num_values_added;
    // Since the whole list is discarded, all added values count as removed.
    num_removed = list.size();
    list.clear();
  }
  return num_removed;
}

std::size_t Shard::removeAll(const Bytes& key, Callables::Predicate predicate) {
  return removeValues(getUnique(key), predicate, false);
}

std::size_t Shard::removeAllEqual(const Bytes& key, const Bytes& value) {
  return removeAll(key, [&value](const Bytes& current_value) {
    return current_value == value;
  });
}

bool Shard::removeFirst(const Bytes& key, Callables::Predicate predicate) {
  return removeValues(getUnique(key), predicate, true);
}

bool Shard::removeFirstEqual(const Bytes& key, const Bytes& value) {
  return removeFirst(key, [&value](const Bytes& current_value) {
    return current_value == value;
  });
}

std::size_t Shard::replaceAll(const Bytes& key, Callables::Function map) {
  return replaceValues(getUnique(key), map, false);
}

std::size_t Shard::replaceAllEqual(const Bytes& key, const Bytes& old_value,
                                   const Bytes& new_value) {
  return replaceAll(key, [&old_value, &new_value](const Bytes& current_value) {
    return (current_value == old_value) ? new_value.toString() : std::string();
  });
}

bool Shard::replaceFirst(const Bytes& key, Callables::Function map) {
  return replaceValues(getUnique(key), map, true);
}

bool Shard::replaceFirstEqual(const Bytes& key, const Bytes& old_value,
                              const Bytes& new_value) {
  return replaceFirst(key,
                      [&old_value, &new_value](const Bytes& current_value) {
    return (current_value == old_value) ? new_value.toString() : std::string();
  });
}

void Shard::forEachKey(Callables::Procedure action,
                       bool ignore_empty_lists) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  for (const auto& entry : map_) {
    SharedList list(*entry.second, *store_, std::try_to_lock);
    if (list && !(ignore_empty_lists && list.empty())) {
      action(entry.first);
    }
  }
}

void Shard::forEachValue(const Bytes& key, Callables::Procedure action) const {
  auto iter = get(key);
  while (iter.hasNext()) {
    action(iter.next());
  }
}

void Shard::forEachValue(const Bytes& key, Callables::Predicate action) const {
  auto iter = get(key);
  while (iter.hasNext()) {
    if (!action(iter.next())) {
      break;
    }
  }
}

void Shard::forEachEntry(Callables::BinaryProcedure action,
                         bool ignore_empty_lists) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  store_->adviseAccessPattern(Store::AccessPattern::WILLNEED);
  for (const auto& entry : map_) {
    SharedList list(*entry.second, *store_, std::try_to_lock);
    if (list && !(ignore_empty_lists && list.empty())) {
      action(entry.first, ListIterator(std::move(list)));
    }
  }
  store_->adviseAccessPattern(Store::AccessPattern::NORMAL);
}

Shard::Stats Shard::getStats() const {
  Stats stats = stats_;
  const auto store_stats = store_->getStats();
  stats.block_size = store_stats.block_size;
  stats.num_blocks = store_stats.num_blocks;
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  for (const auto& entry : map_) {
    const auto& key = entry.first;
    SharedList list(*entry.second, *store_, std::try_to_lock);
    if (list) {
      ++stats.num_keys;
      stats.num_values_put += list.head().num_values_added;
      stats.num_values_rmd += list.head().num_values_removed;
      stats.key_size_avg += key.size();
      stats.key_size_max = mt::max(stats.key_size_max, key.size());
      stats.key_size_min = stats.key_size_min
                               ? mt::min(stats.key_size_min, key.size())
                               : key.size();
      stats.list_size_avg += list.size();
      stats.list_size_max = mt::max(stats.list_size_max, list.size());
      stats.list_size_min = stats.list_size_min
                                ? mt::min(stats.list_size_min, list.size())
                                : list.size();
    }
  }
  if (stats.num_keys) {
    stats.key_size_avg /= stats.num_keys;
    stats.list_size_avg /= stats.num_keys;
  }
  return stats;
}

void Shard::forEachEntry(const boost::filesystem::path& prefix,
                         Callables::BinaryProcedure action) {
  Arena arena;
  Store::Options store_options;
  store_options.readonly = true;
  Store store(getNameOfValuesFile(prefix.string()), store_options);
  store.adviseAccessPattern(Store::AccessPattern::WILLNEED);
  const auto stats = Stats::readFromFile(getNameOfStatsFile(prefix.string()));
  const auto stream = mt::fopen(getNameOfKeysFile(prefix.string()), "r");
  for (std::size_t i = 0; i != stats.num_keys; ++i) {
    const auto entry = Entry::readFromStream(stream.get(), &arena);
    List list(entry.head());
    action(entry.key(), ListIterator(SharedList(list, store)));
  }
}

std::string Shard::getNameOfKeysFile(const std::string& prefix) {
  return prefix + ".keys";
}

std::string Shard::getNameOfStatsFile(const std::string& prefix) {
  return prefix + ".stats";
}

std::string Shard::getNameOfValuesFile(const std::string& prefix) {
  return prefix + ".values";
}

SharedList Shard::getShared(const Bytes& key) const {
  List* list = nullptr;
  {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    const auto iter = map_.find(key);
    if (iter != map_.end()) {
      list = iter->second.get();
    }
  }
  // `mutex_` is unlocked now.
  return list ? SharedList(*list, *store_) : SharedList();
}

UniqueList Shard::getUnique(const Bytes& key) {
  mt::Check::isFalse(isReadOnly(),
                     "Attempt to get write access to read-only list");

  List* list = nullptr;
  {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    const auto iter = map_.find(key);
    if (iter != map_.end()) {
      list = iter->second.get();
    }
  }
  // `mutex_` is unlocked now.
  return list ? UniqueList(list, store_.get(), &arena_) : UniqueList();
}

UniqueList Shard::getUniqueOrCreate(const Bytes& key) {
  mt::Check::isFalse(isReadOnly(),
                     "Attempt to get write access to read-only list");
  mt::Check::isLessEqual(key.size(), Limits::maxKeySize(),
                         "Reject key of %d bytes because it's too big",
                         key.size());

  List* list = nullptr;
  {
    std::lock_guard<boost::shared_mutex> lock(mutex_);
    const auto emplace_result = map_.emplace(key, std::unique_ptr<List>());
    if (emplace_result.second) {
      // Overrides the inserted key with a new deep copy.
      const auto new_key_data = arena_.allocate(key.size());
      std::memcpy(new_key_data, key.data(), key.size());
      const auto old_key_ptr = const_cast<Bytes*>(&emplace_result.first->first);
      *old_key_ptr = Bytes(new_key_data, key.size());
      emplace_result.first->second.reset(new List());
    }
    const auto iter = emplace_result.first;
    list = iter->second.get();
  }
  // `mutex_` is unlocked now.
  return UniqueList(list, store_.get(), &arena_);
}

} // namespace internal
} // namespace multimap
