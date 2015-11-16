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

#include <limits>
#include <boost/filesystem/operations.hpp>
#include "multimap/internal/System.hpp"

namespace multimap {
namespace internal {

namespace {

const char* STORE_FILE_SUFFIX = ".store";
const char* TABLE_FILE_SUFFIX = ".table";

typedef std::pair<Bytes, List::Head> Entry;

Entry readEntryFromFile(std::FILE* file, Arena* arena) {
  std::int32_t key_size;
  System::read(file, &key_size, sizeof key_size);
  const auto key_data = arena->allocate(key_size);
  System::read(file, key_data, key_size);
  const auto head = List::Head::readFromFile(file);
  return std::make_pair(Bytes(key_data, key_size), head);
}

void writeEntryToFile(const Entry& entry, std::FILE* file) {
  const auto& key = entry.first;
  MT_REQUIRE_LE(key.size(), Table::Limits::getMaxKeySize());

  const std::int32_t key_size = key.size();
  System::write(file, &key_size, sizeof key_size);
  System::write(file, key.data(), key.size());
  entry.second.writeToFile(file);
}

Table::Stats readStatsFromTail(std::FILE* file) {
  Table::Stats stats;
  System::seek(file, -(sizeof stats), SEEK_END);
  System::read(file, &stats, sizeof stats);
  return stats;
}

void writeStatsToTail(const Table::Stats& stats, std::FILE* file) {
  System::seek(file, 0, SEEK_END);
  System::write(file, &stats, sizeof stats);
}

} // namespace

std::size_t Table::Limits::getMaxKeySize() {
  return Varint::Limits::MAX_N4;
}

std::size_t Table::Limits::getMaxValueSize() {
  return List::Limits::getMaxValueSize();
}

Table::Stats& Table::Stats::combine(const Stats& other) {
  num_values_added += other.num_values_added;
  num_values_removed += other.num_values_removed;
  num_values_unowned += other.num_values_unowned;
  if (other.num_keys != 0) {
    key_size_min = std::min(key_size_min, other.key_size_min);
    key_size_max = std::max(key_size_max, other.key_size_max);
    const double total_keys = num_keys + other.num_keys;
    key_size_avg = ((num_keys / total_keys) * key_size_avg) +
                   ((other.num_keys / total_keys) * other.key_size_avg);
    // new_avg = (weight * avg'old) + (weight'other * avg'other)
    list_size_min = std::min(list_size_min, other.list_size_min);
    list_size_max = std::max(list_size_max, other.list_size_max);
    list_size_avg = ((num_keys / total_keys) * list_size_avg) +
                    ((other.num_keys / total_keys) * other.list_size_avg);
    // new_avg = (weight * avg'old) + (weight'other * avg'other)
    num_keys += other.num_keys;
  }
  return *this;
}

Table::Stats Table::Stats::combine(const Stats& a, const Stats& b) {
  return Stats(a).combine(b);
}

Table::Stats Table::Stats::fromProperties(const mt::Properties& properties) {
  Stats stats;
  stats.num_keys = std::stoul(properties.at("num_keys"));
  stats.num_values_added = std::stoul(properties.at("num_values_put"));
  stats.num_values_removed = std::stoul(properties.at("num_values_removed"));
  stats.num_values_unowned = std::stoul(properties.at("num_values_unowned"));
  stats.key_size_min = std::stoul(properties.at("key_size_min"));
  stats.key_size_max = std::stoul(properties.at("key_size_max"));
  stats.key_size_avg = std::stoul(properties.at("key_size_avg"));
  stats.list_size_min = std::stoul(properties.at("list_size_min"));
  stats.list_size_max = std::stoul(properties.at("list_size_max"));
  stats.list_size_avg = std::stoul(properties.at("list_size_avg"));
  return stats;
}

mt::Properties Table::Stats::toProperties() const {
  mt::Properties properties;
  properties["num_keys"] = std::to_string(num_keys);
  properties["num_values_put"] = std::to_string(num_values_added);
  properties["num_values_removed"] = std::to_string(num_values_removed);
  properties["num_values_unowned"] = std::to_string(num_values_unowned);
  properties["key_size_min"] = std::to_string(key_size_min);
  properties["key_size_max"] = std::to_string(key_size_max);
  properties["key_size_avg"] = std::to_string(key_size_avg);
  properties["list_size_min"] = std::to_string(list_size_min);
  properties["list_size_max"] = std::to_string(list_size_max);
  properties["list_size_avg"] = std::to_string(list_size_avg);
  return properties;
}

Table::Table(const boost::filesystem::path& filepath_prefix)
    : Table(filepath_prefix, Options()) {}

Table::Table(const boost::filesystem::path& filepath_prefix,
             const Options& options) {
  const auto table_filepath = filepath_prefix.string() + TABLE_FILE_SUFFIX;
  const auto store_filepath = filepath_prefix.string() + STORE_FILE_SUFFIX;

  if (boost::filesystem::is_regular_file(table_filepath)) {
    mt::check(!options.error_if_exists, "Already exists");

    const mt::AutoCloseFile file(std::fopen(table_filepath.c_str(), "r"));
    MT_ASSERT_NOT_NULL(file.get());

    old_stats_ = readStatsFromTail(file.get());
    System::seek(file.get(), 0, SEEK_SET);
    for (std::size_t i = 0; i != old_stats_.num_keys; ++i) {
      const auto entry = readEntryFromFile(file.get(), &arena_);
      map_[entry.first].reset(new List(entry.second));
    }
  } else if (options.create_if_missing) {
    // Nothing to do here.
  } else {
    mt::throwRuntimeError2("Does not exist '%s'", table_filepath.c_str());
  }

  Store::Options store_opts;
  store_opts.block_size = options.block_size;
  store_opts.buffer_size = options.buffer_size;
  store_opts.create_if_missing = options.create_if_missing;
  store_opts.error_if_exists = options.error_if_exists;
  store_.reset(new Store(store_filepath, store_opts));
  prefix_ = filepath_prefix;
}

Table::~Table() {
  if (!prefix_.empty()) {
    const auto filepath = prefix_.string() + TABLE_FILE_SUFFIX;
    const auto old_filepath = filepath + ".old";
    if (boost::filesystem::is_regular_file(filepath)) {
      boost::filesystem::rename(filepath, old_filepath);
    }

    const mt::AutoCloseFile file(std::fopen(filepath.c_str(), "w"));
    MT_ASSERT_NOT_NULL(file.get());
    const auto status = std::setvbuf(file.get(), nullptr, _IOFBF, mt::MiB(1));
    MT_ASSERT_IS_ZERO(status);

    Stats stats;
    for (const auto& entry : map_) {
      const auto list = entry.second.get();
      if (list->is_locked()) {
        System::log(__PRETTY_FUNCTION__)
            << "List is still locked, data is possibly lost. Key was "
            << entry.first.toString() << '\n';
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
        stats.key_size_min = std::min(stats.key_size_min, entry.first.size());
        stats.list_size_avg += list->size();
        stats.list_size_max = std::max(stats.list_size_max, list->size());
        stats.list_size_min = std::min(stats.list_size_min, list->size());
        writeEntryToFile(Entry(entry.first, list->head()), file.get());
      }
    }

    if (stats.num_keys != 0) {
      stats.key_size_avg /= stats.num_keys;
      stats.list_size_avg /= stats.num_keys;
    }

    stats.num_values_unowned += old_stats_.num_values_unowned;
    writeStatsToTail(stats, file.get());

    if (boost::filesystem::is_regular_file(old_filepath)) {
      const auto status = boost::filesystem::remove(old_filepath);
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
  mt::check(key.size() <= Limits::getMaxKeySize(),
            "Reject key because it's too large.");

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

std::size_t Table::getBlockSize() const { return store_->getBlockSize(); }

Table::Stats Table::getStats() const {
  Stats stats;
  forEachEntry([&stats](const Bytes& key, SharedList&& list) {
    ++stats.num_keys;
    stats.num_values_added += list.head().num_values_added;
    stats.num_values_removed += list.head().num_values_removed;
    stats.key_size_min = std::min(stats.key_size_min, key.size());
    stats.key_size_max = std::max(stats.key_size_max, key.size());
    stats.key_size_avg += key.size();
    stats.list_size_min = std::min(stats.list_size_min, list.size());
    stats.list_size_max = std::max(stats.list_size_max, list.size());
    stats.list_size_avg += list.size();
  });
  if (stats.num_keys != 0) {
    stats.key_size_avg /= stats.num_keys;
    stats.list_size_avg /= stats.num_keys;
  }
  stats.num_values_unowned = old_stats_.num_values_unowned;
  return stats;
}

} // namespace internal
} // namespace multimap
