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

TableFile::Entry TableFile::ReadEntryFromStream(std::istream& stream) {
  std::uint16_t key_size;
  stream.read(reinterpret_cast<char*>(&key_size), sizeof key_size);
  const auto key_data = new byte[key_size];
  stream.read(reinterpret_cast<char*>(key_data), key_size);
  auto head = List::Head::ReadFromStream(stream);
  return std::make_pair(Bytes(key_data, key_size), std::move(head));
}

void TableFile::WriteEntryToStream(const Entry& entry, std::ostream& stream) {
  WriteEntryToStream(entry.first, entry.second, stream);
}

void TableFile::WriteEntryToStream(const Bytes& key, const List::Head& head,
                                   std::ostream& stream) {
  assert(key.size() <= std::numeric_limits<std::uint16_t>::max());
  const std::uint16_t key_size = key.size();
  stream.write(reinterpret_cast<const char*>(&key_size), sizeof key_size);
  stream.write(static_cast<const char*>(key.data()), key_size);
  head.WriteToStream(stream);
}

Table::~Table() {
  internal::System::Log() << "Table::~Table BEGIN\n";
  FlushAllLists();
  if (!table_file_.empty()) {
    // TODO Make background thread that writes ther table file periodically.
    WriteMapToFile(map_, table_file_);
    for (const auto& entry : map_) {
      delete[] static_cast<const char*>(entry.first.data());
    }
  }
  internal::System::Log() << "Table::~Table END\n";
}

std::unique_ptr<Table> Table::Open(const boost::filesystem::path& filepath) {
  std::unique_ptr<Table> table(new Table());
  table->map_ = ReadMapFromFile(filepath);
  table->table_file_ = filepath;
  return std::move(table);
}

std::unique_ptr<Table> Table::Open(const boost::filesystem::path& filepath,
                                   const Callbacks::CommitBlock& commit_block) {
  auto table = Open(filepath);
  table->set_commit_block(commit_block);
  return table;
}

std::unique_ptr<Table> Table::Create(const boost::filesystem::path& filepath) {
  assert(!boost::filesystem::exists(filepath));
  std::unique_ptr<Table> table(new Table());
  table->table_file_ = filepath;
  return std::move(table);
}

std::unique_ptr<Table> Table::Create(
    const boost::filesystem::path& filepath,
    const Callbacks::CommitBlock& commit_block) {
  auto table = Create(filepath);
  table->set_commit_block(commit_block);
  return table;
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
  List* list = nullptr;
  {
    std::lock_guard<std::mutex> lock(map_mutex_);
    const auto insert_result = map_.insert(std::make_pair(key, nullptr));
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

// TODO This does not work if the list is checked out.
bool Table::Delete(const Bytes& key) {
  std::lock_guard<std::mutex> lock(map_mutex_);
  const auto iter = map_.find(key);
  if (iter != map_.end()) {
    delete[] static_cast<const char*>(iter->first.data());
    map_.erase(iter);
    return true;
  }
  return false;
}

bool Table::Contains(const Bytes& key) const {
  std::lock_guard<std::mutex> lock(map_mutex_);
  return map_.find(key) != map_.end();
}

void Table::ForEachKey(KeyProcedure procedure) const {
  std::lock_guard<std::mutex> lock(map_mutex_);
  for (const auto& pair : map_) {
    procedure(pair.first);
  }
}

void Table::ForEachList(ListProcedure procedure) const {
  std::lock_guard<std::mutex> lock(map_mutex_);
  for (auto& pair : map_) {
    if (pair.second->TryLockShared()) {
      procedure(*pair.second);
      pair.second->UnlockShared();
    }
  }
}

std::map<std::string, std::string> Table::GetProperties() const {
  std::uint64_t num_values_total = 0;
  std::uint64_t num_values_deleted = 0;
  std::lock_guard<std::mutex> lock(map_mutex_);
  for (const auto& pair : map_) {
    if (pair.second->TryLockShared()) {
      num_values_total += pair.second->chead().num_values_total;
      num_values_deleted += pair.second->chead().num_values_deleted;
      pair.second->UnlockShared();
    }
  }
  return {{"num-keys", std::to_string(map_.size())},
          {"num-values-deleted", std::to_string(num_values_deleted)},
          {"num-values-total", std::to_string(num_values_total)}};
}

void Table::FlushLists(double min_load_factor) {
  if (commit_block_) {
    std::lock_guard<std::mutex> lock(map_mutex_);  // TODO Remove?
    for (const auto& entry : map_) {
      auto list = entry.second.get();
      if (list->TryLockUnique()) {
        if (list->block().load_factor() >= min_load_factor) {
          list->Flush(commit_block_);
        }
        list->UnlockUnique();
      }
    }
  }
}

void Table::FlushAllLists() {
  internal::System::Log() << "Table::FlushAllLists BEGIN\n";
  FlushLists(0);
  internal::System::Log() << "Table::FlushAllLists END\n";
}

const Callbacks::CommitBlock& Table::get_commit_block() const {
  return commit_block_;
}

void Table::set_commit_block(const Callbacks::CommitBlock& commit_block) {
  commit_block_ = commit_block;
}

Table::Map Table::ReadMapFromFile(const boost::filesystem::path& from) {
  Map map;
  std::ifstream stream(from.string(), std::ios_base::binary);
  assert(stream.is_open());
  std::uint32_t num_keys;
  stream.read(reinterpret_cast<char*>(&num_keys), sizeof num_keys);
  assert(stream.good());
  for (std::size_t i = 0; i != num_keys; ++i) {
    auto entry = TableFile::ReadEntryFromStream(stream);
    std::unique_ptr<List> list(new List(std::move(entry.second)));
    map.insert(std::make_pair(entry.first, std::move(list)));
    assert(stream.good());
  }
  return map;
}

void Table::WriteMapToFile(const Map& map, const boost::filesystem::path& to) {
  internal::System::Log() << "Table::WriteMapToFile BEGIN\n";
  assert(boost::filesystem::exists(to.parent_path()));
  const auto stream = std::fopen(to.c_str(), "wb");
  assert(stream != nullptr);

  // Resize internal buffer to 10 MiB.
  auto ret = std::setvbuf(stream, nullptr, _IOFBF, 1024 * 1024 * 10);
  assert(ret == 0);

  const std::uint32_t num_keys = map.size();
  System::Write(stream, &num_keys, sizeof num_keys);

  std::uint32_t num_entries_written = 0;
  for (const auto& entry : map) {
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
  internal::System::Log() << "Table::WriteMapToFile END\n";
}

}  // namespace internal
}  // namespace multimap
