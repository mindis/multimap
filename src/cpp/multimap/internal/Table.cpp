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

#include <algorithm>
#include <cstdio>
#include <boost/filesystem/operations.hpp>
#include <boost/thread/locks.hpp>
#include "multimap/internal/AutoCloseFile.hpp"
#include "multimap/internal/System.hpp"

namespace multimap {
namespace internal {

namespace {

typedef std::pair<Bytes, List::Head> Entry;

Entry ReadEntryFromFile(std::FILE* file, Arena* arena) {
  std::uint16_t key_size;
  System::Read(file, &key_size, sizeof key_size);
  const auto key_data = arena->Allocate(key_size);
  System::Read(file, key_data, key_size);
  const auto head = List::Head::ReadFromFile(file);
  return std::make_pair(Bytes(key_data, key_size), head);
}

void WriteEntryToFile(const Entry& entry, std::FILE* file) {
  assert(entry.first.size() <= std::numeric_limits<std::uint16_t>::max());
  const std::uint16_t key_size = entry.first.size();
  System::Write(file, &key_size, sizeof key_size);
  System::Write(file, entry.first.data(), key_size);
  entry.second.WriteToFile(file);
}

}  // namespace

std::map<std::string, std::string> Table::Stats::ToMap() const {
  return ToMap("");
}

std::map<std::string, std::string> Table::Stats::ToMap(
    const std::string& prefix) const {
  auto prefix_copy = prefix;
  if (!prefix_copy.empty()) {
    prefix_copy.push_back('.');
  }
  std::map<std::string, std::string> map;
  map[prefix_copy + "key_size_avg"] = std::to_string(key_size_avg);
  map[prefix_copy + "key_size_max"] = std::to_string(key_size_max);
  map[prefix_copy + "key_size_min"] = std::to_string(key_size_min);
  map[prefix_copy + "list_size_avg"] = std::to_string(list_size_avg);
  map[prefix_copy + "list_size_max"] = std::to_string(list_size_max);
  map[prefix_copy + "list_size_min"] = std::to_string(list_size_min);
  map[prefix_copy + "num_keys"] = std::to_string(num_keys);
  map[prefix_copy + "num_lists_empty"] = std::to_string(num_lists_empty);
  map[prefix_copy + "num_lists_locked"] = std::to_string(num_lists_locked);
  map[prefix_copy + "num_values_deleted"] = std::to_string(num_values_deleted);
  map[prefix_copy + "num_values_total"] = std::to_string(num_values_total);
  return map;
}

Table::Table(const boost::filesystem::path& file) : Table(file, false) {}

Table::Table(const boost::filesystem::path& file, bool create_if_missing) {
  Open(file, create_if_missing);
}

Table::~Table() { Close(); }

void Table::Open(const boost::filesystem::path& file) { Open(file, false); }

void Table::Open(const boost::filesystem::path& file, bool create_if_missing) {
  assert(path_.empty());

  if (boost::filesystem::is_regular_file(file)) {
    const AutoCloseFile stream(std::fopen(file.c_str(), "r"));
    Check(stream.get(), "Table: Could not open '%s'.", file.c_str());

    std::uint32_t num_keys;
    System::Read(stream.get(), &num_keys, sizeof num_keys);
    for (std::size_t i = 0; i != num_keys; ++i) {
      const auto entry = ReadEntryFromFile(stream.get(), &arena_);
      map_.emplace(entry.first, std::unique_ptr<List>(new List(entry.second)));
    }

  } else if (create_if_missing) {
    const AutoCloseFile stream(std::fopen(file.c_str(), "w"));
    Check(stream.get(), "Table: Could not create '%s'.", file.c_str());

    std::uint32_t num_keys = 0;
    System::Write(stream.get(), &num_keys, sizeof num_keys);

  } else {
    internal::Throw("Table: No such file '%s'.", file.c_str());
  }
  path_ = file;
}

void Table::Close() {
  if (path_.empty()) return;

  const boost::shared_lock<boost::shared_mutex> lock(mutex_);

  const auto backup_path = path_.string() + ".old";
  auto status = std::rename(path_.c_str(), backup_path.c_str());
  assert(status == 0);

  const AutoCloseFile file(std::fopen(path_.c_str(), "w"));
  assert(file.get());
  status = std::setvbuf(file.get(), nullptr, _IOFBF, 1 << 7);  // 10 MiB
  assert(status == 0);

  const std::uint32_t num_keys = map_.size();
  System::Write(file.get(), &num_keys, sizeof num_keys);

  std::uint32_t num_keys_written = 0;
  for (const auto& entry : map_) {
    const auto list = entry.second.get();
    if (list->TryLockUnique()) {
      if (!list->empty()) {
        list->Flush(commit_block_);
        WriteEntryToFile(Entry(entry.first, list->chead()), file.get());
        ++num_keys_written;
      }
      list->UnlockUnique();
    } else {
      System::Log(__PRETTY_FUNCTION__)
          << "List is still locked and could not be flushed. Key was "
          << entry.first.ToString() << '\n';
    }
  }

  if (num_keys_written != map_.size()) {
    std::rewind(file.get());
    System::Write(file.get(), &num_keys_written, sizeof num_keys_written);
  }

  status = std::remove(backup_path.c_str());
  assert(status == 0);

  arena_.Reset();
  path_.clear();
  map_.clear();
}

SharedListLock Table::GetShared(const Bytes& key) const {
  const List* list = nullptr;
  {
    const boost::shared_lock<boost::shared_mutex> lock(mutex_);
    const auto iter = map_.find(key);
    if (iter != map_.end()) {
      list = iter->second.get();
    }
  }
  return list ? SharedListLock(*list) : SharedListLock();
}

UniqueListLock Table::GetUnique(const Bytes& key) const {
  List* list = nullptr;
  {
    const boost::shared_lock<boost::shared_mutex> lock(mutex_);
    const auto iter = map_.find(key);
    if (iter != map_.end()) {
      list = iter->second.get();
    }
  }
  return list ? UniqueListLock(list) : UniqueListLock();
}

UniqueListLock Table::GetUniqueOrCreate(const Bytes& key) {
  Check(key.size() <= max_key_size(),
        "%s: Reject key because its size of %u bytes exceeds the allowed "
        "maximum of %u bytes.",
        __PRETTY_FUNCTION__, key.size(), max_key_size());

  List* list = nullptr;
  {
    const std::lock_guard<boost::shared_mutex> lock(mutex_);
    const auto emplace_result = map_.emplace(key, std::unique_ptr<List>());
    if (emplace_result.second) {
      // Overrides the inserted key with a new deep copy.
      const auto new_key_data = arena_.Allocate(key.size());
      std::memcpy(new_key_data, key.data(), key.size());
      const auto old_key_ptr = const_cast<Bytes*>(&emplace_result.first->first);
      *old_key_ptr = Bytes(new_key_data, key.size());
      emplace_result.first->second.reset(new List());
    }
    const auto iter = emplace_result.first;
    list = iter->second.get();
  }
  return UniqueListLock(list);
}

void Table::ForEachKey(Callables::Procedure procedure) const {
  const boost::shared_lock<boost::shared_mutex> lock(mutex_);
  for (const auto& entry : map_) {
    SharedListLock lock(*entry.second);
    if (!lock.clist()->empty()) {
      procedure(entry.first);
    }
  }
}

void Table::FlushAllLists() const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  for (const auto& entry : map_) {
    auto list = entry.second.get();
    // TODO Use UniqueListLock(List*, std::try_to_lock_t) when available.
    if (list->TryLockUnique()) {
      list->Flush(commit_block_);
      list->UnlockUnique();
    }
  }
}

Table::Stats Table::GetStats() const {
  const boost::shared_lock<boost::shared_mutex> lock(mutex_);
  Stats stats;
  stats.num_keys = map_.size();
  std::size_t key_size_total = 0;
  for (const auto& entry : map_) {
    const auto& key = entry.first;
    const auto list = entry.second.get();
    stats.key_size_min = std::min(key.size(), stats.key_size_min);
    stats.key_size_max = std::max(key.size(), stats.key_size_max);
    key_size_total += key.size();
    if (list->TryLockShared()) {
      if (list->empty()) {
        ++stats.num_lists_empty;
      } else {
        stats.list_size_min = std::min(list->size(), stats.list_size_min);
        stats.list_size_max = std::max(list->size(), stats.list_size_max);
      }
      stats.num_values_total += list->chead().num_values_total;
      stats.num_values_deleted += list->chead().num_values_deleted;
      list->UnlockShared();
    } else {
      ++stats.num_lists_locked;
    }
  }
  if (stats.num_keys != 0) {
    stats.key_size_avg = key_size_total / stats.num_keys;
    stats.list_size_avg = stats.num_values_total / stats.num_keys;
  }
  return stats;
}

const Callbacks::CommitBlock& Table::get_commit_block_callback() const {
  return commit_block_;
}

void Table::set_commit_block_callback(const Callbacks::CommitBlock& callback) {
  commit_block_ = callback;
}

}  // namespace internal
}  // namespace multimap
