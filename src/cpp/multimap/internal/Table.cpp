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

#include "multimap/internal/Table.hpp"

#include <boost/filesystem/operations.hpp>

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
    mt::read(stream, &key_size, sizeof key_size);
    const auto key_data = arena->allocate(key_size);
    mt::read(stream, key_data, key_size);
    auto head = List::Head::readFromStream(stream);
    return Entry(Bytes(key_data, key_size), std::move(head));
  }

  void writeToStream(std::FILE* stream) const {
    MT_REQUIRE_LE(key().size(), Table::Limits::getMaxKeySize());
    const std::int32_t key_size = key().size();
    mt::write(stream, &key_size, sizeof key_size);
    mt::write(stream, key().data(), key().size());
    head().writeToStream(stream);
  }
};

std::uint64_t computeChecksum(const Table::Stats& stats) {
  auto copy = stats;
  copy.checksum = 0;
  return mt::crc32(&copy, sizeof copy);
}

} // namespace

std::size_t Table::Limits::getMaxKeySize() { return Varint::Limits::MAX_N4; }

std::size_t Table::Limits::getMaxValueSize() {
  return List::Limits::getMaxValueSize();
}

const std::vector<std::string>& Table::Stats::names() {
  static std::vector<std::string> names = {
    "block_size",       "num_blocks",         "num_keys",
    "num_values_added", "num_values_removed", "num_values_unowned",
    "key_size_min",     "key_size_max",       "key_size_avg",
    "list_size_min",    "list_size_max",      "list_size_avg"
  };
  return names;
}

Table::Stats Table::Stats::readFromFile(const boost::filesystem::path& file) {
  Stats stats;
  const auto stream = mt::open(file.c_str(), "r");
  mt::check(stream.get(), "Could not open '%s' for reading", file.c_str());
  mt::read(stream.get(), &stats, sizeof stats);
  mt::check(stats.checksum == computeChecksum(stats), "Sanity check failed");
  return stats;
}

void Table::Stats::writeToFile(const boost::filesystem::path& file) const {
  const auto stream = mt::open(file.c_str(), "w");
  mt::check(stream.get(), "Could not create '%s' for writing", file.c_str());
  auto copy = *this;
  copy.checksum = computeChecksum(*this);
  mt::write(stream.get(), &copy, sizeof copy);
}

Table::Stats Table::Stats::fromProperties(const mt::Properties& properties) {
  Stats stats;
  stats.block_size = std::stoul(properties.at("block_size"));
  stats.num_blocks = std::stoul(properties.at("num_blocks"));
  stats.num_keys = std::stoul(properties.at("num_keys"));
  stats.num_values_added = std::stoul(properties.at("num_values_added"));
  stats.num_values_removed = std::stoul(properties.at("num_values_removed"));
  stats.num_values_unowned = std::stoul(properties.at("num_values_unowned"));
  stats.key_size_min = std::stoul(properties.at("key_size_min"));
  stats.key_size_max = std::stoul(properties.at("key_size_max"));
  stats.key_size_avg = std::stoul(properties.at("key_size_avg"));
  stats.list_size_min = std::stoul(properties.at("list_size_min"));
  stats.list_size_max = std::stoul(properties.at("list_size_max"));
  stats.list_size_avg = std::stoul(properties.at("list_size_avg"));
  // stats.checksum is left default initialized.
  return stats;
}

mt::Properties Table::Stats::toProperties() const {
  mt::Properties properties;
  properties["block_size"] = std::to_string(block_size);
  properties["num_blocks"] = std::to_string(num_blocks);
  properties["num_keys"] = std::to_string(num_keys);
  properties["num_values_added"] = std::to_string(num_values_added);
  properties["num_values_removed"] = std::to_string(num_values_removed);
  properties["num_values_unowned"] = std::to_string(num_values_unowned);
  properties["key_size_min"] = std::to_string(key_size_min);
  properties["key_size_max"] = std::to_string(key_size_max);
  properties["key_size_avg"] = std::to_string(key_size_avg);
  properties["list_size_min"] = std::to_string(list_size_min);
  properties["list_size_max"] = std::to_string(list_size_max);
  properties["list_size_avg"] = std::to_string(list_size_avg);
  // stats.checksum is not exposed.
  return properties;
}

std::vector<std::uint64_t> Table::Stats::toVector() const {
  return { block_size,       num_blocks,         num_keys,
           num_values_added, num_values_removed, num_values_unowned,
           key_size_min,     key_size_max,       key_size_avg,
           list_size_min,    list_size_max,      list_size_avg };
}

Table::Stats Table::Stats::total(const std::vector<Stats>& stats) {
  Table::Stats total;
  for (const auto& stat : stats) {
    if (total.block_size == 0) {
      total.block_size = stat.block_size;
    } else {
      MT_ASSERT_EQ(total.block_size, stat.block_size);
    }
    total.num_blocks += stat.num_blocks;
    total.num_keys += stat.num_keys;
    total.num_values_added += stat.num_values_added;
    total.num_values_removed += stat.num_values_removed;
    total.num_values_unowned += stat.num_values_unowned;
    if (total.key_size_min == 0) {
      total.key_size_min = stat.key_size_min;
    } else {
      total.key_size_min = std::min(total.key_size_min, stat.key_size_min);
    }
    total.key_size_max = std::max(total.key_size_max, stat.key_size_max);
    if (total.list_size_min == 0) {
      total.list_size_min = stat.list_size_min;
    } else {
      total.list_size_min = std::min(total.list_size_min, stat.list_size_min);
    }
    total.list_size_max = std::max(total.list_size_max, stat.list_size_max);
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

Table::Stats Table::Stats::max(const std::vector<Stats>& stats) {
  Table::Stats max;
  for (const auto& stat : stats) {
    max.block_size = std::max(max.block_size, stat.block_size);
    max.num_blocks = std::max(max.num_blocks, stat.num_blocks);
    max.num_keys = std::max(max.num_keys, stat.num_keys);
    max.num_values_added =
        std::max(max.num_values_added, stat.num_values_added);
    max.num_values_removed =
        std::max(max.num_values_removed, stat.num_values_removed);
    max.num_values_unowned =
        std::max(max.num_values_unowned, stat.num_values_unowned);
    max.key_size_min = std::max(max.key_size_min, stat.key_size_min);
    max.key_size_max = std::max(max.key_size_max, stat.key_size_max);
    max.key_size_avg = std::max(max.key_size_avg, stat.key_size_avg);
    max.list_size_min = std::max(max.list_size_min, stat.list_size_min);
    max.list_size_max = std::max(max.list_size_max, stat.list_size_max);
    max.list_size_avg = std::max(max.list_size_avg, stat.list_size_avg);
  }
  return max;
}

Table::Table(const boost::filesystem::path& file_prefix)
    : Table(file_prefix, Options()) {}

Table::Table(const boost::filesystem::path& file_prefix,
             const Options& options) {
  const auto stats_file = getNameOfStatsFile(file_prefix.string());
  if (boost::filesystem::is_regular_file(stats_file)) {
    mt::check(!options.error_if_exists, "Already exists");

    stats_ = Stats::readFromFile(stats_file);
    const auto keys_file = getNameOfKeysFile(file_prefix.string());
    const auto stream = mt::open(keys_file.c_str(), "r");
    MT_ASSERT_NOT_NULL(stream.get());
    for (std::size_t i = 0; i != stats_.num_keys; ++i) {
      const auto entry = Entry::readFromStream(stream.get(), &arena_);
      map_[entry.key()].reset(new List(entry.head()));
    }

  } else if (options.create_if_missing) {
    // Nothing to do here.

  } else {
    mt::fail("Could not open '%s' because it does not exist",
             stats_file.c_str());
  }

  Store::Options store_opts;
  store_opts.block_size = options.block_size;
  store_opts.buffer_size = options.buffer_size;
  store_opts.create_if_missing = options.create_if_missing;
  store_opts.error_if_exists = options.error_if_exists;
  store_opts.readonly = options.readonly;
  const auto values_file = getNameOfValuesFile(file_prefix.string());
  store_.reset(new Store(values_file, store_opts));
  prefix_ = file_prefix;
}

Table::~Table() {
  if (!prefix_.empty() && !isReadOnly()) {
    const auto keys_file = getNameOfKeysFile(prefix_.string());
    const auto old_keys_file = keys_file + ".old";
    if (boost::filesystem::is_regular_file(keys_file)) {
      boost::filesystem::rename(keys_file, old_keys_file);
    }

    const mt::AutoCloseFile stream(std::fopen(keys_file.c_str(), "w"));
    MT_ASSERT_NOT_NULL(stream.get());

    Stats stats;
    const auto store_stats = store_->getStats();
    stats.block_size = store_stats.block_size;
    stats.num_blocks = store_stats.num_blocks;
    stats.num_values_unowned = stats_.num_values_unowned;
    for (const auto& entry : map_) {
      const auto list = entry.second.get();
      if (list->is_locked()) {
        mt::log() << "The list with the key '" << entry.first.toString()
                  << "' was still locked when shutting down."
                  << " Recent updates of the list may be lost.\n";
      }
      // We do not skip or even throw if a list is still locked to prevent data
      // loss. However, this causes a race which could let the program crash...
      list->flush(store_.get());
      if (list->empty()) {
        stats.num_values_unowned += list->head().num_values_removed;
        // An existing list that is empty consists only of removed values.
        // Because such lists are skipped on closing these values have no owner
        // anymore, i.e. no reference pointing to them, but they still exist
        // physically in the data file, i.e. the store.
      } else {
        ++stats.num_keys;
        stats.num_values_added += list->head().num_values_added;
        stats.num_values_removed += list->head().num_values_removed;
        stats.key_size_avg += entry.first.size();
        stats.key_size_max = std::max(stats.key_size_max, entry.first.size());
        if (stats.key_size_min == 0) {
          stats.key_size_min = entry.first.size();
        } else {
          stats.key_size_min = std::min(stats.key_size_min, entry.first.size());
        }
        stats.list_size_avg += list->size();
        stats.list_size_max = std::max(stats.list_size_max, list->size());
        if (stats.list_size_min == 0) {
          stats.list_size_min = list->size();
        } else {
          stats.list_size_min = std::min(stats.list_size_min, list->size());
        }
        Entry(entry.first, list->head()).writeToStream(stream.get());
      }
    }

    if (stats.num_keys != 0) {
      stats.key_size_avg /= stats.num_keys;
      stats.list_size_avg /= stats.num_keys;
    }
    stats.writeToFile(getNameOfStatsFile(prefix_.string()));

    if (boost::filesystem::is_regular_file(old_keys_file)) {
      const auto status = boost::filesystem::remove(old_keys_file);
      MT_ASSERT_TRUE(status);
    }
  }
}

SharedList Table::getShared(const Bytes& key) const {
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

UniqueList Table::getUnique(const Bytes& key) {
  mt::check(!isReadOnly(), "Attempt to get write access to read-only table");

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

UniqueList Table::getUniqueOrCreate(const Bytes& key) {
  mt::check(!isReadOnly(), "Attempt to get write access to read-only table");
  mt::check(key.size() <= Limits::getMaxKeySize(),
            "Reject key of %d bytes because it's too big.", key.size());

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

void Table::forEachKey(Callables::Procedure action) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  for (const auto& entry : map_) {
    SharedList list(*entry.second, *store_, std::try_to_lock);
    if (list && !list.empty()) {
      action(entry.first);
    }
  }
}

void Table::forEachEntry(BinaryProcedure action) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  store_->adviseAccessPattern(Store::AccessPattern::WILLNEED);
  for (const auto& entry : map_) {
    SharedList list(*entry.second, *store_, std::try_to_lock);
    if (list && !list.empty()) {
      action(entry.first, std::move(list));
    }
  }
  store_->adviseAccessPattern(Store::AccessPattern::NORMAL);
}

Table::Stats Table::getStats() const {
  Stats stats;
  const auto store_stats = store_->getStats();
  stats.block_size = store_stats.block_size;
  stats.num_blocks = store_stats.num_blocks;
  stats.num_values_unowned = stats_.num_values_unowned;
  forEachEntry([&stats](const Bytes& key, SharedList&& list) {
    ++stats.num_keys;
    stats.num_values_added += list.head().num_values_added;
    stats.num_values_removed += list.head().num_values_removed;
    if (stats.key_size_min == 0) {
      stats.key_size_min = key.size();
    } else {
      stats.key_size_min = std::min(stats.key_size_min, key.size());
    }
    stats.key_size_max = std::max(stats.key_size_max, key.size());
    stats.key_size_avg += key.size();
    if (stats.list_size_min == 0) {
      stats.list_size_min = list.size();
    } else {
      stats.list_size_min = std::min(stats.list_size_min, list.size());
    }
    stats.list_size_max = std::max(stats.list_size_max, list.size());
    stats.list_size_avg += list.size();
  });
  if (stats.num_keys != 0) {
    stats.key_size_avg /= stats.num_keys;
    stats.list_size_avg /= stats.num_keys;
  }
  return stats;
}

std::string Table::getNameOfKeysFile(const std::string& prefix) {
  return prefix + ".keys";
}

std::string Table::getNameOfStatsFile(const std::string& prefix) {
  return prefix + ".stats";
}

std::string Table::getNameOfValuesFile(const std::string& prefix) {
  return prefix + ".values";
}

} // namespace internal
} // namespace multimap
