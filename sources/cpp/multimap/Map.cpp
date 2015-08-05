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

#include "multimap/Map.hpp"

#include <cstring>
#include <fstream>
#include <boost/filesystem/operations.hpp>
#include "multimap/internal/BlockPool.hpp"
#include "multimap/internal/Block.hpp"
#include "multimap/internal/Check.hpp"
#include "multimap/internal/System.hpp"

namespace multimap {

namespace {

const char* kNameOfLockFile = "multimap.lock";
const char* kNameOfDataFile = "multimap.data";
const char* kNameOfTableFile = "multimap.table";

}  // namespace

Map::Map() {}

Map::~Map() {
  table_.FlushAllLists();
  table_.WriteToFile(directory_lock_guard_.directory() / kNameOfTableFile);
}

Map::Map(const boost::filesystem::path& directory, const Options& options) {
  Open(directory, options);
}

void Map::Open(const boost::filesystem::path& directory,
               const Options& options) {
  const auto absolute_directory = boost::filesystem::absolute(directory);
  internal::Check(boost::filesystem::is_directory(absolute_directory),
                  "The path '%s' does not refer to a directory.",
                  absolute_directory.c_str());

  directory_lock_guard_.Lock(absolute_directory, kNameOfLockFile);
  const auto data_filepath = absolute_directory / kNameOfDataFile;
  const auto table_filepath = absolute_directory / kNameOfTableFile;
  const auto data_file_exists = boost::filesystem::exists(data_filepath);
  const auto table_file_exists = boost::filesystem::exists(table_filepath);

  const auto only_one_file_exists = data_file_exists != table_file_exists;
  if (only_one_file_exists) {
    internal::Throw(
        "The Multimap in '%s' is corrupt because '%s' is missing.",
        data_file_exists ? table_filepath.c_str() : data_filepath.c_str());
  }

  internal::Check(options.block_pool_memory >= options.block_size,
                  "options.block_pool_memory is too small.\n"
                  "Visit 'http://multimap.io/#limitations' for more details.");

  InitCallbacks();
  options_ = options;
  const auto num_blocks = options.block_pool_memory / options.block_size;
  block_pool_.Init(num_blocks, options.block_size);
  data_file_.Open(data_filepath, callbacks_.deallocate_blocks,
                  options.create_if_missing, options.block_size);
  table_.set_commit_block(callbacks_.commit_block);
  if (table_file_exists) {
    table_.InitFromFile(table_filepath);
  }
}

void Map::Put(const Bytes& key, const Bytes& value) {
  table_.GetUniqueOrCreate(key).list()->Add(value, callbacks_.allocate_block,
                                            callbacks_.commit_block);
}

Map::ConstIter Map::Get(const Bytes& key) const {
  return ConstIter(table_.GetShared(key), callbacks_);
}

Map::Iter Map::GetMutable(const Bytes& key) {
  return Iter(table_.GetUnique(key), callbacks_);
}

bool Map::Contains(const Bytes& key) const {
  const auto list_lock = table_.GetShared(key);
  return list_lock.has_list() && !list_lock.clist()->empty();
}

std::size_t Map::Delete(const Bytes& key) {
  std::size_t num_deleted = 0;
  auto list_lock = table_.GetUnique(key);
  if (list_lock.has_list()) {
    num_deleted = list_lock.clist()->size();
    list_lock.list()->Clear();
  }
  return num_deleted;
}

bool Map::DeleteFirst(const Bytes& key, Callables::Predicate predicate) {
  return Delete(key, predicate, Match::kOne);
}

bool Map::DeleteFirstEqual(const Bytes& key, const Bytes& value) {
  return DeleteFirst(key, [&value](const Bytes& current_value) {
    return current_value == value;
  });
}

std::size_t Map::DeleteAll(const Bytes& key, Callables::Predicate predicate) {
  return Delete(key, predicate, Match::kAll);
}

std::size_t Map::DeleteAllEqual(const Bytes& key, const Bytes& value) {
  return DeleteAll(key, [&value](const Bytes& current_value) {
    return current_value == value;
  });
}

bool Map::ReplaceFirst(const Bytes& key, Callables::Function function) {
  return Replace(key, function, Match::kOne);
}

bool Map::ReplaceFirstEqual(const Bytes& key, const Bytes& old_value,
                            const Bytes& new_value) {
  return ReplaceFirst(key,
                      [&old_value, &new_value](const Bytes& current_value) {
    return (current_value == old_value) ? new_value.ToString() : std::string();
  });
}

std::size_t Map::ReplaceAll(const Bytes& key, Callables::Function function) {
  return Replace(key, function, Match::kAll);
}

std::size_t Map::ReplaceAllEqual(const Bytes& key, const Bytes& old_value,
                                 const Bytes& new_value) {
  return ReplaceAll(key, [&old_value, &new_value](const Bytes& current_value) {
    return (current_value == old_value) ? new_value.ToString() : std::string();
  });
}

void Map::ForEachKey(Callables::Procedure procedure) const {
  table_.ForEachKey(procedure);
}

// TODO Make thread-safe.
// TODO Property names need improvement.
std::map<std::string, std::string> Map::GetProperties() const {
  auto props = table_.GetProperties();
  props["block-load-factor"] =
      std::to_string(data_file_.super_block().load_factor_total /
                     data_file_.super_block().num_blocks);
  props["block-pool-num-blocks"] = std::to_string(block_pool_.num_blocks());
  props["block-pool-num-blocks-free"] =
      std::to_string(block_pool_.num_blocks_free());
  props["block-pool-memory"] = std::to_string(block_pool_.memory());
  props["block-size"] = std::to_string(block_pool_.block_size());
  props["max-key-size"] = std::to_string(max_key_size());
  props["max-value-size"] = std::to_string(max_value_size());
  props["num-blocks-written"] =
      std::to_string(data_file_.super_block().num_blocks);
  return props;
}

void Map::PrintProperties() const {
  for (const auto& property : GetProperties()) {
    std::cout << property.first << ": " << property.second << '\n';
  }
}

std::size_t Map::max_key_size() const { return table_.max_key_size(); }

std::size_t Map::max_value_size() const {
  return options_.block_size - internal::Block::kSizeOfValueSizeField;
}

const char* Map::name_of_data_file() { return kNameOfDataFile; }

const char* Map::name_of_table_file() { return kNameOfTableFile; }

void Map::Copy(const boost::filesystem::path& from,
               const boost::filesystem::path& to) {
  Copy(from, to, -1);
}

void Map::Copy(const boost::filesystem::path& from,
               const boost::filesystem::path& to, std::size_t new_block_size) {
  Copy(from, to, Callables::Compare(), new_block_size);
}

void Map::Copy(const boost::filesystem::path& from,
               const boost::filesystem::path& to,
               const Callables::Compare& compare) {
  Copy(from, to, compare, -1);
}

void Map::Copy(const boost::filesystem::path& from,
               const boost::filesystem::path& to,
               const Callables::Compare& compare, std::size_t new_block_size) {
  //  using namespace internal;
  //  namespace fs = boost::filesystem;
  //  const auto table_path = from / kNameOfTableFile;
  //  const auto data_path = from / kNameOfDataFile;
  //  const auto err_msg1 = "Does not exist or is not a regular file: %s.";
  //  const auto err_msg2 = "Directory does not exist: %s.";
  //  const auto err_mgs3 = "Directory is not empty: %s.";
  //  Check(fs::is_regular_file(table_path), err_msg1, table_path.c_str());
  //  Check(fs::is_regular_file(data_path), err_msg1, data_path.c_str());
  //  Check(fs::is_directory(to), err_msg2, to.c_str());
  //  Check(fs::is_empty(to), err_mgs3, to.c_str());
  //  internal::System::DirectoryLockGuard lock_from(from);
  //  internal::System::DirectoryLockGuard lock_to(to);

  //  const auto new_table_path = to / kNameOfTableFile;
  //  const auto new_data_path = to / kNameOfDataFile;

  //  std::ifstream table_ifs(table_path.string(), std::ios_base::binary);
  //  std::ofstream new_table_ofs(new_table_path.string(),
  //  std::ios_base::binary);

  //  DataFile data_file(data_path);
  //  DataFile new_data_file(new_data_path, new_block_size);

  //  std::uint32_t num_keys;
  //  table_ifs.read(reinterpret_cast<char*>(&num_keys), sizeof num_keys);
  //  assert(table_ifs.good());
  //  for (std::size_t i = 0; i != num_keys; ++i) {
  //    const auto entry = TableFile::ReadEntryFromStream(table_ifs);
  //    const auto head = entry.second;
  //    const auto new_head = List::Copy(head, data_file, &new_data_file,
  //    compare);
  //    if (new_head.num_values_total != 0) {
  //      const auto key = entry.first;
  //      const auto new_entry = TableFile::Entry(key, new_head);
  //      TableFile::WriteEntryToStream(new_entry, new_table_ofs);
  //    }
  //    delete[] static_cast<const char*>(entry.first.data());
  //  }
}

std::size_t Map::Delete(const Bytes& key, Callables::Predicate predicate,
                        Match match) {
  std::size_t num_deleted = 0;
  auto iter = GetMutable(key);
  for (iter.SeekToFirst(); iter.HasValue(); iter.Next()) {
    if (predicate(iter.GetValue())) {
      iter.DeleteValue();
      ++num_deleted;
      iter.Next();
      break;
    }
  }
  if (match == Match::kAll) {
    for (; iter.HasValue(); iter.Next()) {
      if (predicate(iter.GetValue())) {
        iter.DeleteValue();
        ++num_deleted;
      }
    }
  }
  return num_deleted;
}

std::size_t Map::Replace(const Bytes& key, Callables::Function function,
                         Match match) {
  std::vector<std::string> updated_values;
  auto iter = GetMutable(key);
  for (iter.SeekToFirst(); iter.HasValue(); iter.Next()) {
    const auto updated_value = function(iter.GetValue());
    if (!updated_value.empty()) {
      updated_values.emplace_back(updated_value);
      iter.DeleteValue();
      iter.Next();
      break;
    }
  }
  if (match == Match::kAll) {
    for (; iter.HasValue(); iter.Next()) {
      const auto updated_value = function(iter.GetValue());
      if (!updated_value.empty()) {
        updated_values.emplace_back(updated_value);
        iter.DeleteValue();
      }
    }
  }
  if (!updated_values.empty()) {
    auto list_lock = iter.ReleaseListLock();
    for (const auto& value : updated_values) {
      list_lock.list()->Add(value, callbacks_.allocate_block,
                            callbacks_.commit_block);
    }
  }
  return updated_values.size();
}

void Map::InitCallbacks() {
  // Thread-safe: yes.
  callbacks_.allocate_block = [this]() {
    static std::mutex mutex;
    const std::lock_guard<std::mutex> lock(mutex);
    auto new_block = block_pool_.Pop();
    if (!new_block.has_data()) {
      if (options_.verbose) {
        internal::System::Log("Multimap") << "block pool ran out of blocks\n";
      }
      double load_factor = 0.8;
      while (block_pool_.empty()) {
        table_.FlushLists(load_factor);
        data_file_.Flush();
        load_factor -= 0.2;
      }
      new_block = block_pool_.Pop();
      assert(new_block.has_data());
    }
    return new_block;
  };

  // Thread-safe: yes.
  callbacks_.deallocate_block =
      [this](internal::Block&& block) { block_pool_.Push(std::move(block)); };

  // Thread-safe: yes.
  callbacks_.deallocate_blocks = [this](std::vector<internal::Block>* blocks) {
    block_pool_.Push(blocks);
  };

  // Thread-safe: yes.
  callbacks_.commit_block = [this](internal::Block&& block) {
    return data_file_.Append(std::move(block));
  };

  // Thread-safe: yes.
  callbacks_.update_block =
      [this](std::uint32_t block_id, const internal::Block& block) {
    data_file_.Write(block_id, block);
  };

  // Thread-safe: yes.
  callbacks_.request_block =
      [this](std::uint32_t block_id,
             internal::Block* block) { data_file_.Read(block_id, block); };
}

}  // namespace multimap
