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
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <boost/crc.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem/operations.hpp>
#include "multimap/internal/Check.hpp"
#include "multimap/internal/Base64.hpp"

namespace multimap {

namespace {

const char* kNameOfLockFile = "multimap.lock";
const char* kNameOfPropsFile = "multimap.properties";
const char* kNameOfStoreFile = "multimap.store";
const char* kNameOfTableFile = "multimap.table";

bool IsPowerOfTwo(std::size_t value) { return (value & (value - 1)) == 0; }

void CheckOptions(const Options& options) {
  using internal::Check;

  const auto bs = "options.block_size";
  Check(options.block_size != 0, "%s must be positive.", bs);
  Check(IsPowerOfTwo(options.block_size), "%s must be a power of two.", bs);

  const auto wbs = "options.write_buffer_size";
  Check(options.write_buffer_size != 0, "%s must be positive.", wbs);
  Check(options.write_buffer_size >= options.block_size,
        "%s must be greater than or equal to %s.", wbs, bs);
}

void CheckVersion(std::uint32_t client_major, std::uint32_t client_minor) {
  internal::Check(client_major == kMajorVersion,
                  "Version check failed. The Multimap you are trying to open "
                  "was created with version %u.%u of the library. Your "
                  "installed version is %u.%u which is not compatible.",
                  client_major, client_minor, kMajorVersion, kMinorVersion);
}

std::size_t Checksum(const std::string& str) {
  boost::crc_32_type crc;
  crc.process_bytes(str.data(), str.size());
  return crc.checksum();
}

std::string Join(const std::map<std::string, std::string>& properties) {
  std::string joined;
  const auto is_space = [](char c) { return std::isspace(c); };
  for (const auto& entry : properties) {
    const auto key = boost::trim_copy(entry.first);
    const auto val = boost::trim_copy(entry.second);
    if (std::any_of(key.begin(), key.end(), is_space)) continue;
    if (std::any_of(val.begin(), val.end(), is_space)) continue;
    if (key.empty() || val.empty()) continue;
    joined.append(key);
    joined.push_back('=');
    joined.append(val);
    joined.push_back('\n');
  }
  return joined;
}

std::map<std::string, std::string> ReadPropertiesFromFile(
    const boost::filesystem::path& file) {
  std::ifstream ifs(file.string());
  internal::Check(ifs, "Could not open '%s'.", file.c_str());

  std::string line;
  std::map<std::string, std::string> properties;
  while (std::getline(ifs, line)) {
    if (line.empty()) continue;
    const auto pos_of_delim = line.find('=');
    if (pos_of_delim == std::string::npos) continue;
    const auto key = line.substr(0, pos_of_delim);
    const auto val = line.substr(pos_of_delim + 1);
    // We don't make any checks here, because external modification
    // of key and val will be captured during checksum verification.
    properties[key] = val;
  }

  const auto actual_checksum_str = properties["checksum"];
  internal::Check(!actual_checksum_str.empty(),
                  "Properties file '%s' is missing checksum.", file.c_str());
  const auto actual_checksum = std::stoul(actual_checksum_str);

  properties.erase("checksum");
  const auto expected_checksum = Checksum(Join(properties));
  internal::Check(actual_checksum == expected_checksum,
                  "Metadata in '%s' has wrong checksum.", file.c_str());
  return properties;
}

void WritePropertiesToFile(const std::map<std::string, std::string>& properties,
                           const boost::filesystem::path& file) {
  assert(properties.count("checksum") == 0);

  std::ofstream ofs(file.string());
  internal::Check(ofs, "Could not create '%s'.", file.c_str());

  const auto content = Join(properties);
  ofs << content << "checksum=" << Checksum(content) << '\n';
}

}  // namespace

Map::Map() {}

Map::~Map() {
  table_.FlushAllLists();
  const auto filename = directory_lock_guard_.path() / kNameOfPropsFile;
  WritePropertiesToFile(GetProperties(), filename);
}

Map::Map(const boost::filesystem::path& directory, const Options& options) {
  Open(directory, options);
}

void Map::Open(const boost::filesystem::path& directory,
               const Options& options) {
  CheckOptions(options);
  const auto absolute_directory = boost::filesystem::absolute(directory);
  internal::Check(boost::filesystem::is_directory(absolute_directory),
                  "The path '%s' does not refer to a directory.",
                  absolute_directory.c_str());

  directory_lock_guard_.Lock(absolute_directory, kNameOfLockFile);
  const auto path_to_props = absolute_directory / kNameOfPropsFile;
  const auto path_to_store = absolute_directory / kNameOfStoreFile;
  const auto path_to_table = absolute_directory / kNameOfTableFile;
  const auto props_exists = boost::filesystem::is_regular_file(path_to_props);
  const auto store_exists = boost::filesystem::is_regular_file(path_to_store);
  const auto table_exists = boost::filesystem::is_regular_file(path_to_table);

  const auto all_files_exist = props_exists && store_exists && table_exists;
  const auto no_file_exists = !props_exists && !store_exists && !table_exists;

  options_ = options;
  if (all_files_exist) {
    internal::Check(!options_.error_if_exists,
                    "A Multimap in '%s' does already exist.",
                    absolute_directory.c_str());

    const auto properties = ReadPropertiesFromFile(path_to_props);
    const auto pos = properties.find("store.block_size");
    assert(pos != properties.end());

    options_.block_size = std::stoul(pos->second);
    const auto num_blocks = options_.write_buffer_size / options_.block_size;
    block_pool_.Init(num_blocks, options_.block_size);

    internal::DataFile::Options store_options;
    store_options.block_size = options_.block_size;
    store_.Open(path_to_store, store_options);
    table_.Open(path_to_table);

  } else if (no_file_exists) {
    internal::Check(options_.create_if_missing, "No Multimap found in '%s'.",
                    absolute_directory.c_str());

    const auto num_blocks = options_.write_buffer_size / options_.block_size;
    block_pool_.Init(num_blocks, options_.block_size);

    internal::DataFile::Options data_file_opts;
    data_file_opts.block_size = options_.block_size;
    data_file_opts.create_if_missing = true;
    store_.Open(path_to_store, data_file_opts);
    table_.Open(path_to_table, true);

  } else {
    internal::Throw("The Multimap in '%s' is corrupt because '%s' is missing.",
                    absolute_directory.c_str(),
                    store_exists ? (props_exists ? path_to_table.c_str()
                                                 : path_to_props.c_str())
                                 : path_to_store.c_str());
  }

  InitCallbacks();
  table_.set_commit_block_callback(callbacks_.commit_block);
}

void Map::Put(const Bytes& key, const Bytes& value) {
  table_.GetUniqueOrCreate(key).list()->Add(value, callbacks_.new_block,
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
  return Delete(key, predicate, false);
}

bool Map::DeleteFirstEqual(const Bytes& key, const Bytes& value) {
  return DeleteFirst(key, [&value](const Bytes& current_value) {
    return current_value == value;
  });
}

std::size_t Map::DeleteAll(const Bytes& key, Callables::Predicate predicate) {
  return Delete(key, predicate, true);
}

std::size_t Map::DeleteAllEqual(const Bytes& key, const Bytes& value) {
  return DeleteAll(key, [&value](const Bytes& current_value) {
    return current_value == value;
  });
}

bool Map::ReplaceFirst(const Bytes& key, Callables::Function function) {
  return Replace(key, function, false);
}

bool Map::ReplaceFirstEqual(const Bytes& key, const Bytes& old_value,
                            const Bytes& new_value) {
  return ReplaceFirst(key,
                      [&old_value, &new_value](const Bytes& current_value) {
    return (current_value == old_value) ? new_value.ToString() : std::string();
  });
}

std::size_t Map::ReplaceAll(const Bytes& key, Callables::Function function) {
  return Replace(key, function, true);
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

std::map<std::string, std::string> Map::GetProperties() const {
  std::map<std::string, std::string> properties;
  properties["major_version"] = std::to_string(kMajorVersion);
  properties["minor_version"] = std::to_string(kMinorVersion);
  const auto store_stats = store_.GetStats().ToMap("store");
  const auto table_stats = table_.GetStats().ToMap("table");
  properties.insert(store_stats.begin(), store_stats.end());
  properties.insert(table_stats.begin(), table_stats.end());
  return properties;
}

std::size_t Map::max_key_size() const { return table_.max_key_size(); }

std::size_t Map::max_value_size() const {
  return options_.block_size - internal::Block::kSizeOfValueSizeField;
}

std::size_t Map::Delete(const Bytes& key, Callables::Predicate predicate,
                        bool all) {
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
  if (all) {
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
                         bool all) {
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
  if (all) {
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
      list_lock.list()->Add(value, callbacks_.new_block,
                            callbacks_.commit_block);
    }
  }
  return updated_values.size();
}

void Map::InitCallbacks() {
  // Thread-safe: yes.
  callbacks_.new_block = [this]() {
    static std::mutex mutex;
    const std::lock_guard<std::mutex> lock(mutex);
    auto new_block = block_pool_.Pop();
    if (!new_block.has_data()) {
      if (options_.verbose) {
        internal::System::Log("Map") << "write buffer out of blocks\n";
      }
      double load_factor = 0.75;
      while (block_pool_.empty()) {
        table_.FlushLists(load_factor);
        load_factor = std::pow(load_factor, 2);
      }
      new_block = block_pool_.Pop();
      assert(new_block.has_data());
    }
    return new_block;
  };

  // Thread-safe: yes.
  callbacks_.commit_block = [this](internal::Block&& block) {
    const auto id = store_.Append(block);
    block_pool_.Push(std::move(block));
    return id;
  };

  // Thread-safe: yes.
  callbacks_.replace_blocks =
      [this](const std::vector<internal::BlockWithId>& blocks) {
    store_.Write(blocks);
  };

  // Thread-safe: yes.
  callbacks_.request_blocks =
      [this](std::vector<internal::BlockWithId>* blocks,
             internal::Arena* arena) { store_.Read(blocks, arena); };
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
  //  using namespace internal;
  //  System::DirectoryLockGuard from_lock(from, kNameOfLockFile);
  //  System::DirectoryLockGuard to_lock(to, kNameOfLockFile);

  //  const auto from_table_filename = from / kNameOfTableFile;
  //  const auto from_table_file = std::fopen(from_table_filename.c_str(),
  //  "rb");
  //  Check(from_table_file != nullptr, "Could not open '%s'.",
  //        from_table_filename.c_str());

  //  const auto from_data_filename = from / kNameOfDataFile;
  //  internal::DataFile::Options from_data_opts;
  ////  from_data_opts.block_size = from_meta.block_size;  // TODO
  //  DataFile from_data_file(from_data_filename, from_data_opts);
  ////  BlockPool from_block_pool(1, from_meta.block_size);

  //  const auto to_table_filename = to / kNameOfTableFile;
  //  const auto to_table_file = std::fopen(to_table_filename.c_str(), "wb");
  //  Check(to_table_file != nullptr, "Could not create '%s'.",
  //        to_table_filename.c_str());

  //  const auto to_data_filename = to / kNameOfDataFile;
  ////  const auto to_block_size = (new_block_size !=
  /// static_cast<std::size_t>(-1))
  ////                                 ? new_block_size
  ////                                 : from_meta.block_size;
  //  internal::DataFile::Options to_data_opts;
  ////  to_data_opts.block_size = to_block_size;
  //  to_data_opts.error_if_exists = true;
  //  DataFile to_data_file(to_data_filename, to_data_opts);

  //  Arena arena;
  //  std::uint32_t num_keys;
  //  System::Read(from_table_file, &num_keys, sizeof num_keys);
  //  System::Write(to_table_file, &num_keys, sizeof num_keys);
  //  for (std::size_t i = 0; i != num_keys; ++i) {
  //    const auto entry = TableFile::ReadEntryFromFile(from_table_file,
  //    &arena);
  //    const auto new_head =
  //        List::Copy(entry.second, from_data_file, &from_block_pool,
  //                   &to_data_file, &to_block_pool, compare);
  //    const TableFile::Entry new_entry(entry.first, new_head);
  //    TableFile::WriteEntryToFile(new_entry, to_table_file);
  //  }

  //  std::fclose(to_table_file);
  //  std::fclose(from_table_file);
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
