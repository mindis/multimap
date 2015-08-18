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
#include <boost/thread/locks.hpp>
#include "multimap/internal/AutoCloseFile.hpp"
#include "multimap/internal/System.hpp"

namespace multimap {
namespace internal {

void TableFile::CreateInitialFile(const boost::filesystem::path& path) {
  const auto file = std::fopen(path.c_str(), "w");
  Check(file != nullptr, "Could not create '%s'.", path.c_str());
  AutoCloseFile auto_file(file);
  const std::uint32_t num_keys = 0;
  System::Write(auto_file.get(), &num_keys, sizeof num_keys);
}

TableFile::Entry TableFile::ReadEntryFromFile(std::FILE* file, Arena* arena) {
  std::uint16_t key_size;
  System::Read(file, &key_size, sizeof key_size);
  const auto key_data = arena->Allocate(key_size);
  System::Read(file, key_data, key_size);
  const auto head = List::Head::ReadFromStream(file);
  return std::make_pair(Bytes(key_data, key_size), head);
}

void TableFile::WriteEntryToFile(const Entry& entry, std::FILE* file) {
  assert(entry.first.size() <= std::numeric_limits<std::uint16_t>::max());
  const std::uint16_t key_size = entry.first.size();
  System::Write(file, &key_size, sizeof key_size);
  System::Write(file, entry.first.data(), key_size);
  entry.second.WriteToStream(file);
}

Table::Table() : backing_file_(nullptr) {}

Table::~Table() {
  if (backing_file_) {
    // Resize buffer to 10 MiB.
    const auto status = std::setvbuf(backing_file_, nullptr, _IOFBF, 1 << 7);
    assert(status == 0);

    // Write num_keys header.
    std::rewind(backing_file_);
    const std::uint32_t num_keys = map_.size();
    System::Write(backing_file_, &num_keys, sizeof num_keys);
  }

  std::uint32_t num_keys_written = 0;
  for (const auto& entry : map_) {
    const auto list = entry.second.get();
    // assert(!list->locked());
    // TODO Use UniqueListLock(List*, std::try_to_lock_t) when available.
    if (list->TryLockUnique()) {
      if (!list->empty()) {
        list->Flush(commit_block_);
        if (backing_file_) {
          TableFile::WriteEntryToFile(
                TableFile::Entry(entry.first, list->chead()), backing_file_);
          ++num_keys_written;
        }
      }
      list->UnlockUnique();
    } else {
      System::Log("Table::~Table") << "List is still locked. Data is lost.";
    }
  }

  if (backing_file_ && (num_keys_written != map_.size())) {
    std::rewind(backing_file_);
    System::Write(backing_file_, &num_keys_written, sizeof num_keys_written);
  }
}

void Table::InitFromFile(std::FILE* file) {
  map_.clear();
  arena_.Reset();
  std::uint32_t num_keys;
  System::Read(file, &num_keys, sizeof num_keys);
  for (std::size_t i = 0; i != num_keys; ++i) {
    const auto entry = TableFile::ReadEntryFromFile(file, &arena_);
    map_.emplace(entry.first, std::unique_ptr<List>(new List(entry.second)));
  }
  set_backing_file(file);
}

SharedListLock Table::GetShared(const Bytes& key) const {
  const List* list = nullptr;
  {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
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
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    // map_ is accessed read-only, so a shared_lock is sufficient.
    const auto iter = map_.find(key);
    if (iter != map_.end()) {
      list = iter->second.get();
    }
  }
  return list ? UniqueListLock(list) : UniqueListLock();
}

UniqueListLock Table::GetUniqueOrCreate(const Bytes& key) {
  Check(key.size() <= max_key_size(),
        "Table: Reject key because its size of %u bytes exceeds the allowed "
        "maximum of %u bytes.",
        key.size(), max_key_size());
  List* list = nullptr;
  {
    std::lock_guard<boost::shared_mutex> lock(mutex_);
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
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  for (const auto& entry : map_) {
    SharedListLock lock(*entry.second);
    if (!lock.clist()->empty()) {
      procedure(entry.first);
    }
  }
}

std::map<std::string, std::string> Table::GetProperties() const {
  std::uint64_t num_values_total = 0;
  std::uint64_t num_values_deleted = 0;
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  for (const auto& entry : map_) {
    // TODO Introduce parameter to force waiting.
    if (entry.second->TryLockShared()) {
      num_values_total += entry.second->chead().num_values_total;
      num_values_deleted += entry.second->chead().num_values_deleted;
      entry.second->UnlockShared();
    }
  }
  return {{"num-keys", std::to_string(map_.size())},
          {"num-values-deleted", std::to_string(num_values_deleted)},
          {"num-values-total", std::to_string(num_values_total)}};
}

void Table::FlushLists(double min_load_factor) const {
  boost::shared_lock<boost::shared_mutex> lock(mutex_);
  for (const auto& entry : map_) {
    auto list = entry.second.get();
    // TODO Use UniqueListLock(List*, std::try_to_lock_t) when available.
    if (list->TryLockUnique()) {
      if (list->cblock().load_factor() >= min_load_factor) {
        list->Flush(commit_block_);
      }
      list->UnlockUnique();
    }
  }
}

void Table::FlushAllLists() const { FlushLists(0); }

const std::FILE* Table::get_backing_file() const {
  return backing_file_;
}

void Table::set_backing_file(std::FILE* file) {
  assert(file != nullptr);
  backing_file_ = file;
}

const Callbacks::CommitBlock& Table::get_commit_block_callback() const {
  return commit_block_;
}

void Table::set_commit_block_callback(const Callbacks::CommitBlock& callback) {
  commit_block_ = callback;
}

}  // namespace internal
}  // namespace multimap
