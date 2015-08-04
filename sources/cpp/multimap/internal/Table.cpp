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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdlib>
#include <fstream>
#include <boost/filesystem/operations.hpp>
#include "multimap/internal/System.hpp"
#include "multimap/internal/Varint.hpp"

namespace multimap {
namespace internal {

TableFile::Entry TableFile::ReadEntryFromStream(std::FILE* stream) {
  std::uint16_t key_size;
  System::Read(stream, &key_size, sizeof key_size);
  const auto key_data = new byte[key_size];
  System::Read(stream, key_data, key_size);
  const auto head = List::Head::ReadFromStream(stream);
  return std::make_pair(Bytes(key_data, key_size), head);
}

void TableFile::WriteEntryToStream(const Entry& entry, std::FILE* stream) {
  assert(entry.first.size() <= std::numeric_limits<std::uint16_t>::max());
  const std::uint16_t key_size = entry.first.size();
  System::Write(stream, &key_size, sizeof key_size);
  System::Write(stream, entry.first.data(), key_size);
  entry.second.WriteToStream(stream);
}

Table::Table() {}

Table::Table(const Callbacks::CommitBlock& commit_block) {
  set_commit_block(commit_block);
}

Table::~Table() {
  FlushAllLists();
  for (const auto& entry : map_) {
    delete[] static_cast<const char*>(entry.first.data());
  }
}

SharedListLock Table::GetShared(const Bytes& key) const {
  const List* list = nullptr;
  {
    std::lock_guard<std::mutex> lock(map_mutex_);
    const auto iter = map_.find(key);
    if (iter != map_.end()) {
      list = iter->second.get();
    }
  }
  return list ? SharedListLock(*list) : SharedListLock();
}

UniqueListLock Table::GetUnique(const Bytes& key) {
  List* list = nullptr;
  {
    std::lock_guard<std::mutex> lock(map_mutex_);
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
    std::lock_guard<std::mutex> lock(map_mutex_);
    const auto insert_result = map_.emplace(key, std::unique_ptr<List>());
    if (insert_result.second) {
      // Override the inserted key with a new deep copy.
      auto new_key_data = new byte[key.size()];
      std::memcpy(new_key_data, key.data(), key.size());
      const auto old_key_ptr = const_cast<Bytes*>(&insert_result.first->first);
      *old_key_ptr = Bytes(new_key_data, key.size());
      insert_result.first->second.reset(new List());
    }
    const auto iter = insert_result.first;
    list = iter->second.get();
  }
  return UniqueListLock(list);
}

void Table::ForEachKey(Callables::Procedure procedure) const {
  std::lock_guard<std::mutex> lock(map_mutex_);
  for (const auto& entry : map_) {
    if (!entry.second->empty()) {
      procedure(entry.first);
    }
  }
}

std::map<std::string, std::string> Table::GetProperties() const {
  std::uint64_t num_values_total = 0;
  std::uint64_t num_values_deleted = 0;
  std::lock_guard<std::mutex> lock(map_mutex_);
  for (const auto& entry : map_) {
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

void Table::FlushLists(double min_load_factor) {
  if (commit_block_) {
    std::lock_guard<std::mutex> lock(map_mutex_);
    for (const auto& entry : map_) {
      auto list = entry.second.get();
      // TODO Use ListLock(List*, std::try_to_lock_t) when available.
      if (list->TryLockUnique()) {
        if (list->block().load_factor() >= min_load_factor) {
          list->Flush(commit_block_);
        }
        list->UnlockUnique();
      }
    }
  }
}

void Table::FlushAllLists() { FlushLists(0); }

void Table::InitFromFile(const boost::filesystem::path& path) {
  const auto stream = std::fopen(path.c_str(), "rb");
  Check(stream != nullptr, "Could not open '%s'.", path.c_str());

  map_.clear();
  std::uint32_t num_keys;
  System::Read(stream, &num_keys, sizeof num_keys);
  for (std::size_t i = 0; i != num_keys; ++i) {
    const auto entry = TableFile::ReadEntryFromStream(stream);
    map_.emplace(entry.first, std::unique_ptr<List>(new List(entry.second)));
  }
}

void Table::WriteToFile(const boost::filesystem::path& path) {
  const auto stream = std::fopen(path.c_str(), "wb");
  Check(stream != nullptr, "Could not open or create '%s'.", path.c_str());

  // Resize internal buffer to 10 MiB.
  auto ret = std::setvbuf(stream, nullptr, _IOFBF, 1024 * 1024 * 10);
  assert(ret == 0);

  const std::uint32_t num_keys = map_.size();
  System::Write(stream, &num_keys, sizeof num_keys);

  std::uint32_t num_entries_written = 0;
  for (const auto& entry : map_) {
    const auto list = entry.second.get();
    assert(!list->locked());
    assert(!list->cblock().has_data());
    if (!list->empty()) {
      const auto& key = entry.first;
      const std::uint16_t key_size = key.size();
      System::Write(stream, &key_size, sizeof key_size);
      System::Write(stream, key.data(), key.size());

      const auto& head = list->chead();
      System::Write(stream, &head.num_values_total,
                    sizeof head.num_values_total);
      System::Write(stream, &head.num_values_deleted,
                    sizeof head.num_values_deleted);
      const std::uint16_t ids_size = head.block_ids.size();
      System::Write(stream, &ids_size, sizeof ids_size);
      System::Write(stream, head.block_ids.data(), head.block_ids.size());
      ++num_entries_written;
    }
  }

  if (num_entries_written != num_keys) {
    System::Seek(stream, 0);
    System::Write(stream, &num_entries_written, sizeof num_entries_written);
  }

  ret = std::fclose(stream);
  assert(ret == 0);
}

const Callbacks::CommitBlock& Table::get_commit_block() const {
  return commit_block_;
}

void Table::set_commit_block(const Callbacks::CommitBlock& commit_block) {
  commit_block_ = commit_block;
}

}  // namespace internal
}  // namespace multimap
