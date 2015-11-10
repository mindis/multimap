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
  MT_REQUIRE_LE(key.size(), Table::Limits::max_key_size());

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

std::size_t Table::Limits::max_key_size() {
  return std::numeric_limits<std::int32_t>::max();
}

Table::Stats& Table::Stats::combine(const Stats& other) {
  num_values_put += other.num_values_put;
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
  return fromProperties(properties, "");
}

Table::Stats Table::Stats::fromProperties(const mt::Properties& properties,
                                          const std::string& prefix) {
  auto pfx = prefix;
  if (!pfx.empty()) {
    pfx.push_back('.');
  }
  Stats stats;
  using namespace std;
  stats.num_keys = stoul(properties.at(pfx + "num_keys"));
  stats.num_values_put = stoul(properties.at(pfx + "num_values_put"));
  stats.num_values_removed = stoul(properties.at(pfx + "num_values_removed"));
  stats.num_values_unowned = stoul(properties.at(pfx + "num_values_unowned"));
  stats.key_size_min = stoul(properties.at(pfx + "key_size_min"));
  stats.key_size_max = stoul(properties.at(pfx + "key_size_max"));
  stats.key_size_avg = stoul(properties.at(pfx + "key_size_avg"));
  stats.list_size_min = stoul(properties.at(pfx + "list_size_min"));
  stats.list_size_max = stoul(properties.at(pfx + "list_size_max"));
  stats.list_size_avg = stoul(properties.at(pfx + "list_size_avg"));
  return stats;
}

mt::Properties Table::Stats::toProperties() const { return toProperties(""); }

mt::Properties Table::Stats::toProperties(const std::string& prefix) const {
  auto pfx = prefix;
  if (!pfx.empty()) {
    pfx.push_back('.');
  }
  mt::Properties properties;
  properties[pfx + "num_keys"] = std::to_string(num_keys);
  properties[pfx + "num_values_put"] = std::to_string(num_values_put);
  properties[pfx + "num_values_removed"] = std::to_string(num_values_removed);
  properties[pfx + "num_values_unowned"] = std::to_string(num_values_unowned);
  properties[pfx + "key_size_min"] = std::to_string(key_size_min);
  properties[pfx + "key_size_max"] = std::to_string(key_size_max);
  properties[pfx + "key_size_avg"] = std::to_string(key_size_avg);
  properties[pfx + "list_size_min"] = std::to_string(list_size_min);
  properties[pfx + "list_size_max"] = std::to_string(list_size_max);
  properties[pfx + "list_size_avg"] = std::to_string(list_size_avg);
  return properties;
}

Table::~Table() {
  MT_ASSERT_FALSE(isOpen());
  // Not closing a table manually via close() is a clear error,
  // because then the table isn't flushed and data is definitely lost.
}

Table::Table(const boost::filesystem::path& filepath) { open(filepath); }

void Table::open(const boost::filesystem::path& filepath) {
  MT_REQUIRE_FALSE(isOpen());

  if (boost::filesystem::is_regular_file(filepath)) {
    const mt::AutoCloseFile file(std::fopen(filepath.c_str(), "r"));
    MT_ASSERT_NOT_NULL(file.get());

    old_stats_ = readStatsFromTail(file.get());
    System::seek(file.get(), 0, SEEK_SET);
    for (std::size_t i = 0; i != old_stats_.num_keys; ++i) {
      const auto entry = readEntryFromFile(file.get(), &arena_);
      map_[entry.first].reset(new List(entry.second));
    }
  }
  filepath_ = filepath;

  MT_ENSURE_TRUE(isOpen());
}

Table::Stats Table::close(Store* store) {
  MT_REQUIRE_TRUE(isOpen());

  const auto old_filepath = filepath_.string() + ".old";
  if (boost::filesystem::is_regular_file(filepath_)) {
    boost::filesystem::rename(filepath_, old_filepath);
  }

  const mt::AutoCloseFile file(std::fopen(filepath_.c_str(), "w"));
  assert(file.get());
  const auto status = std::setvbuf(file.get(), nullptr, _IOFBF, mt::MiB(10));
  assert(status == 0);

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
    list->flush(store);
    if (list->empty()) {
      stats.num_values_unowned += list->head().num_values_removed;
      // An existing list that is empty consists only of removed values.
      // Because such lists are skipped on closing these values have no owner
      // anymore, i.e. no reference pointing to them, but they still exist
      // physically in the data file, i.e. the store.
    } else {
      ++stats.num_keys;
      stats.num_values_put += list->head().num_values_added;
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
  filepath_.clear();

  MT_ENSURE_FALSE(isOpen());
  return stats;
}

bool Table::isOpen() const { return !filepath_.empty(); }

SharedListLock Table::getShared(const Bytes& key) const {
  List* list = nullptr;
  {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    const auto iter = map_.find(key);
    if (iter != map_.end()) {
      list = iter->second.get();
    }
  }
  return list ? SharedListLock(*list) : SharedListLock();
}

UniqueListLock Table::getUnique(const Bytes& key) const {
  List* list = nullptr;
  {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    const auto iter = map_.find(key);
    if (iter != map_.end()) {
      list = iter->second.get();
    }
  }
  return list ? UniqueListLock(*list) : UniqueListLock();
}

UniqueListLock Table::getUniqueOrCreate(const Bytes& key) {
  mt::check(key.size() <= Limits::max_key_size(),
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
  return UniqueListLock(*list);
}

void Table::forEachKey(Callables::Procedure action) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  for (const auto& entry : map_) {
    SharedListLock lock(*entry.second);
    if (!lock.list()->empty()) {
      action(entry.first);
    }
  }
}

void Table::forEachEntry(EntryProcedure action) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  for (const auto& entry : map_) {
    SharedListLock list_lock(*entry.second);
    if (!list_lock.list()->empty()) {
      action(entry.first, std::move(list_lock));
    }
  }
}

Table::Stats Table::getStats() const {
  Stats stats;
  forEachEntry([&stats](const Bytes& key, SharedListLock&& list_lock) {
    const auto list = list_lock.list();
    ++stats.num_keys;
    stats.num_values_put += list->head().num_values_added;
    stats.num_values_removed += list->head().num_values_removed;
    stats.key_size_min = std::min(stats.key_size_min, key.size());
    stats.key_size_max = std::max(stats.key_size_max, key.size());
    stats.key_size_avg += key.size();
    stats.list_size_min = std::min(stats.list_size_min, list->size());
    stats.list_size_max = std::max(stats.list_size_max, list->size());
    stats.list_size_avg += list->size();
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
