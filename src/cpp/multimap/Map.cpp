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
#include <algorithm>
#include <fstream>
#include <boost/filesystem/operations.hpp>
#include "multimap/internal/Check.hpp"
#include "multimap/internal/Base64.hpp"

namespace multimap {

namespace {

const char* kNameOfLockFile = "multimap.lock";
const char* kNameOfDataFile = "multimap.data";
const char* kNameOfTableFile = "multimap.table";

}  // namespace

Map::Map() {}

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

  const auto exists = data_file_exists && table_file_exists;
  if (exists && options.error_if_exists) {
    internal::Throw("A Multimap in '%s' does already exist.",
                    absolute_directory.c_str());
  }

  const auto only_one_file_exists = data_file_exists != table_file_exists;
  if (only_one_file_exists) {
    internal::Throw(
        "The Multimap in '%s' is corrupt because '%s' is missing.",
        data_file_exists ? table_filepath.c_str() : data_filepath.c_str());
  }

  internal::Check(options.block_pool_memory >= options.block_size,
                  "options.block_pool_memory is too small.\n"
                  "Visit 'http://multimap.io' for more details.");

  InitCallbacks();
  data_file_.Open(data_filepath, callbacks_.deallocate_blocks,
                  options.create_if_missing, options.block_size);

  // Open table file.
  if (!table_file_exists) {
    internal::TableFile::CreateInitialFile(table_filepath);
  }
  const auto table_file = std::fopen(table_filepath.c_str(), "r+");
  internal::Check(table_file != nullptr, "Could not open '%s'.",
                  table_filepath.c_str());
  table_.InitFromFile(table_file);
  table_.set_commit_block_callback(callbacks_.commit_block);
  table_file_.reset(table_file);

  options_ = options;
  options_.block_size = data_file_.block_size();
  const auto num_blocks = options_.block_pool_memory / options_.block_size;
  block_pool_.Init(num_blocks, options_.block_size);
}

void Map::Put(const Bytes& key, const Bytes& value) {
  table_.GetUniqueOrCreate(key).list()->Add(value, callbacks_.allocate_block,
                                            callbacks_.commit_block);
}

Map::ConstIter Map::Get(const Bytes& key) const {
  return ConstIter(table_.GetShared(key), callbacks_.request_blocks);
}

Map::Iter Map::GetMutable(const Bytes& key) {
  return Iter(table_.GetUnique(key), callbacks_.request_blocks,
              callbacks_.replace_blocks);
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

void Map::ForEachValue(const Bytes& key, Callables::Procedure procedure) const {
  const auto list_lock = table_.GetShared(key);
  if (list_lock.has_list()) {
    list_lock.list()->ForEach(procedure, callbacks_.request_blocks);
  }
}

void Map::ForEachValue(const Bytes& key, Callables::Predicate predicate) const {
  const auto list_lock = table_.GetShared(key);
  if (list_lock.has_list()) {
    list_lock.list()->ForEach(predicate, callbacks_.request_blocks);
  }
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
        internal::System::Log("Map") << "block pool ran out of blocks\n";
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
  callbacks_.deallocate_blocks = [this](std::vector<internal::Block>* blocks) {
    block_pool_.Push(blocks);
  };

  // Thread-safe: yes.
  callbacks_.commit_block = [this](internal::Block&& block) {
    return data_file_.Append(std::move(block));
  };

  // Thread-safe: yes.
  callbacks_.replace_blocks =
      [this](const std::vector<internal::Callbacks::Job>& jobs) {
    data_file_.Write(jobs);
  };

  // Thread-safe: yes.
  callbacks_.request_blocks =
      [this](std::vector<internal::Callbacks::Job>* jobs,
             internal::Arena* arena) { data_file_.Read(jobs, arena); };
}

void Copy(const boost::filesystem::path& from,
          const boost::filesystem::path& to) {
  Copy(from, to, -1);
}

void Copy(const boost::filesystem::path& from,
          const boost::filesystem::path& to, std::size_t new_block_size) {
  Copy(from, to, Callables::Compare(), new_block_size);
}

void Copy(const boost::filesystem::path& from,
          const boost::filesystem::path& to,
          const Callables::Compare& compare) {
  Copy(from, to, compare, -1);
}

// TODO https://bitbucket.org/mtrenkmann/multimap/issues/4
void Copy(const boost::filesystem::path& from,
          const boost::filesystem::path& to, const Callables::Compare& compare,
          std::size_t new_block_size) {
  using namespace internal;
  System::DirectoryLockGuard from_lock(from, kNameOfLockFile);
  System::DirectoryLockGuard to_lock(to, kNameOfLockFile);

  const auto from_table_filename = from / kNameOfTableFile;
  const auto from_table_file = std::fopen(from_table_filename.c_str(), "rb");
  Check(from_table_file != nullptr, "Could not open '%s'.",
        from_table_filename.c_str());

  const auto from_data_filename = from / kNameOfDataFile;
  DataFile from_data_file(from_data_filename);
  BlockPool from_block_pool(1, from_data_file.block_size());

  const auto to_table_filename = to / kNameOfTableFile;
  const auto to_table_file = std::fopen(to_table_filename.c_str(), "wb");
  Check(to_table_file != nullptr, "Could not create '%s'.",
        to_table_filename.c_str());

  const auto to_data_filename = to / kNameOfDataFile;
  const auto to_block_size = (new_block_size != static_cast<std::size_t>(-1))
                                 ? new_block_size
                                 : from_data_file.block_size();
  BlockPool to_block_pool(DataFile::max_block_buffer_size() + 1, to_block_size);
  const auto deallocate_blocks = [&to_block_pool](std::vector<Block>* blocks) {
    to_block_pool.Push(blocks);
  };
  DataFile to_data_file(to_data_filename, deallocate_blocks, true,
                        to_block_size);

  Arena arena;
  std::uint32_t num_keys;
  System::Read(from_table_file, &num_keys, sizeof num_keys);
  System::Write(to_table_file, &num_keys, sizeof num_keys);
  for (std::size_t i = 0; i != num_keys; ++i) {
    const auto entry = TableFile::ReadEntryFromFile(from_table_file, &arena);
    const auto new_head =
        List::Copy(entry.second, from_data_file, &from_block_pool,
                   &to_data_file, &to_block_pool, compare);
    const TableFile::Entry new_entry(entry.first, new_head);
    TableFile::WriteEntryToFile(new_entry, to_table_file);
  }

  std::fclose(to_table_file);
  std::fclose(from_table_file);
}

void Import(const boost::filesystem::path& directory,
            const boost::filesystem::path& file) {
  Options options;
  options.verbose = true;
  Map map(directory, options);
  std::ifstream ifs(file.string());
  internal::Check(ifs, "Cannot open '%s'.", file.c_str());

  std::string base64_key;
  std::string binary_key;
  std::string base64_value;
  std::string binary_value;
  if (ifs >> base64_key) {
    internal::Base64::Decode(base64_key, &binary_key);
    while (ifs) {
      switch (ifs.peek()) {
        case '\n':
        case '\r':
          ifs >> base64_key;
          internal::Base64::Decode(base64_key, &binary_key);
          break;
        case '\f':
        case '\t':
        case '\v':
        case ' ':
          ifs.ignore();
          break;
        default:
          ifs >> base64_value;
          internal::Base64::Decode(base64_value, &binary_value);
          map.Put(binary_key, binary_value);
      }
    }
  }
}

void Export(const boost::filesystem::path& directory,
            const boost::filesystem::path& file) {
  Map map(directory, Options());
  std::ofstream ofs(file.string());
  internal::Check(ofs, "Cannot create '%s'.", file.c_str());

  std::string base64_key;
  std::string base64_value;
  map.ForEachKey([&](const Bytes& binary_key) {
    auto iter = map.Get(binary_key);
    iter.SeekToFirst();
    if (iter.HasValue()) {
      internal::Base64::Encode(binary_key, &base64_key);
      internal::Base64::Encode(iter.GetValue(), &base64_value);
      ofs << base64_key << ' ' << base64_value;
      iter.Next();
    }
    while (iter.HasValue()) {
      internal::Base64::Encode(iter.GetValue(), &base64_value);
      ofs << ' ' << base64_value;
      iter.Next();
    }
    ofs << '\n';
  });
}

}  // namespace multimap
