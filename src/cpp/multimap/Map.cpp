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
#include "multimap/internal/List.hpp"
#include "multimap/internal/thirdparty/mt.hpp"

namespace multimap {

namespace {

const char* NAME_OF_LOCK_FILE = "multimap.lock";
const char* NAME_OF_PROPS_FILE = "multimap.properties";
const char* NAME_OF_TABLE_FILE = "multimap.table";

bool isPowerOfTwo(std::size_t value) { return (value & (value - 1)) == 0; }

void checkOptions(const Options& options) {
  using internal::check;

  const auto bs = "options.block_size";
  check(options.block_size != 0, "%s must be positive.", bs);
  check(isPowerOfTwo(options.block_size), "%s must be a power of two.", bs);
}

void checkVersion(std::uint32_t client_major, std::uint32_t client_minor) {
  internal::check(
      client_major == MAJOR_VERSION && client_minor == MINOR_VERSION,
      "Version check failed. The Multimap you are trying to open "
      "was created with version %u.%u of the library. Your "
      "installed version is %u.%u which is not compatible.",
      client_major, client_minor, MAJOR_VERSION, MINOR_VERSION);
}

std::size_t checksum(const std::string& str) {
  boost::crc_32_type crc;
  crc.process_bytes(str.data(), str.size());
  return crc.checksum();
}

std::string join(const std::map<std::string, std::string>& properties) {
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

std::map<std::string, std::string> readPropertiesFromFile(
    const boost::filesystem::path& file) {
  std::ifstream ifs(file.string());
  internal::check(ifs, "Could not open '%s'.", file.c_str());

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
  internal::check(!actual_checksum_str.empty(),
                  "Properties file '%s' is missing checksum.", file.c_str());
  const auto actual_checksum = std::stoul(actual_checksum_str);

  properties.erase("checksum");
  const auto expected_checksum = checksum(join(properties));
  internal::check(actual_checksum == expected_checksum,
                  "Metadata in '%s' has wrong checksum.", file.c_str());
  return properties;
}

void writePropertiesToFile(const std::map<std::string, std::string>& properties,
                           const boost::filesystem::path& file) {
  assert(properties.count("checksum") == 0);

  std::ofstream ofs(file.string());
  internal::check(ofs, "Could not create '%s'.", file.c_str());

  const auto content = join(properties);
  ofs << content << "checksum=" << checksum(content) << '\n';
}

}  // namespace

Map::Map() {}

Map::~Map() {
  try {
    table_.flushAllListsOrThrowIfLocked();
  } catch (...) {
    std::terminate();
    // The destructor has the precondition that no list is still locked.
  }

  const auto filename = directory_lock_guard_.path() / NAME_OF_PROPS_FILE;
  writePropertiesToFile(getProperties(), filename);
}

Map::Map(const boost::filesystem::path& directory, const Options& options) {
  open(directory, options);
}

void Map::open(const boost::filesystem::path& directory,
               const Options& options) {
  checkOptions(options);
  const auto absolute_directory = boost::filesystem::absolute(directory);
  internal::check(boost::filesystem::is_directory(absolute_directory),
                  "The path '%s' does not refer to a directory.",
                  absolute_directory.c_str());

  directory_lock_guard_.lock(absolute_directory, NAME_OF_LOCK_FILE);
  const auto path_to_props = absolute_directory / NAME_OF_PROPS_FILE;
  const auto path_to_table = absolute_directory / NAME_OF_TABLE_FILE;
  const auto props_exists = boost::filesystem::is_regular_file(path_to_props);
  const auto table_exists = boost::filesystem::is_regular_file(path_to_table);

  const auto all_files_exist = props_exists && table_exists;
  const auto no_file_exists = !props_exists && !table_exists;

  options_ = options;
  if (all_files_exist) {
    internal::check(!options_.error_if_exists,
                    "A Multimap in '%s' does already exist.",
                    absolute_directory.c_str());

    const auto properties = readPropertiesFromFile(path_to_props);
    const auto pos = properties.find("store.block_size");
    assert(pos != properties.end());

    options_.block_size = std::stoul(pos->second);
    block_pool_.init(options_.block_size);

    internal::BlockStore::Options store_options;
    store_options.block_size = options_.block_size;
    block_store_.open(absolute_directory, store_options);
    table_.open(path_to_table);

  } else if (no_file_exists) {
    internal::check(options_.create_if_missing, "No Multimap found in '%s'.",
                    absolute_directory.c_str());

    block_pool_.init(options_.block_size);

    internal::BlockStore::Options store_options;
    store_options.create_if_missing = true;
    store_options.block_size = options_.block_size;
    block_store_.open(absolute_directory, store_options);
    table_.open(path_to_table, true);

  } else {
    mt::throwRuntimeError2(
        "The Multimap in '%s' is corrupt because '%s' is missing.",
        absolute_directory.c_str(),
        props_exists ? path_to_table.c_str() : path_to_props.c_str());
  }

  initCallbacks();
  table_.set_commit_block_callback(callbacks_.commit_block);
}

void Map::put(const Bytes& key, const Bytes& value) {
  table_.getUniqueOrCreate(key).list()->add(key, value, callbacks_.new_block,
                                            callbacks_.commit_block);
}

Map::ConstListIter Map::get(const Bytes& key) const {
  return ConstListIter(table_.getShared(key), callbacks_.request_blocks);
}

Map::ListIter Map::getMutable(const Bytes& key) {
  return ListIter(table_.getUnique(key), callbacks_.request_blocks,
                  callbacks_.replace_blocks);
}

bool Map::contains(const Bytes& key) const {
  const auto list_lock = table_.getShared(key);
  return list_lock.hasList() && !list_lock.clist()->empty();
}

std::size_t Map::remove(const Bytes& key) {
  std::size_t num_deleted = 0;
  auto list_lock = table_.getUnique(key);
  if (list_lock.hasList()) {
    num_deleted = list_lock.clist()->size();
    list_lock.list()->clear();
  }
  return num_deleted;
}

std::size_t Map::removeAll(const Bytes& key,
                           Callables::BytesPredicate predicate) {
  return remove(key, predicate, true);
}

std::size_t Map::removeAllEqual(const Bytes& key, const Bytes& value) {
  return removeAll(key, [&value](const Bytes& current_value) {
    return current_value == value;
  });
}

bool Map::removeFirst(const Bytes& key, Callables::BytesPredicate predicate) {
  return remove(key, predicate, false);
}

bool Map::removeFirstEqual(const Bytes& key, const Bytes& value) {
  return removeFirst(key, [&value](const Bytes& current_value) {
    return current_value == value;
  });
}

std::size_t Map::replaceAll(const Bytes& key,
                            Callables::BytesFunction function) {
  return replace(key, function, true);
}

std::size_t Map::replaceAllEqual(const Bytes& key, const Bytes& old_value,
                                 const Bytes& new_value) {
  return replaceAll(key, [&old_value, &new_value](const Bytes& current_value) {
    return (current_value == old_value) ? new_value.toString() : std::string();
  });
}

bool Map::replaceFirst(const Bytes& key, Callables::BytesFunction function) {
  return replace(key, function, false);
}

bool Map::replaceFirstEqual(const Bytes& key, const Bytes& old_value,
                            const Bytes& new_value) {
  return replaceFirst(key,
                      [&old_value, &new_value](const Bytes& current_value) {
    return (current_value == old_value) ? new_value.toString() : std::string();
  });
}

void Map::forEachEntry(Callables::EntryProcedure procedure) const {
  // Wraps Callables::EntryProcedure into internal::Table::EntryProcedure.
  auto table_entry_proc = [procedure, this](
      const Bytes& key, internal::SharedListLock&& list_lock) {
    ConstListIter iter(std::move(list_lock), callbacks_.request_blocks);
    procedure(key, std::move(iter));
  };
  // We want to iterate all entries in such an order so that the block files
  // `multimap.i` are accessed one by one, and not randomly for each key. This
  // leads to much better caching behavior of the file in question and also
  // works well if the file is mmap()-ped.
  auto compare = [this](const Bytes& key_a, const Bytes& key_b) {
    return block_store_.getFileId(key_a) < block_store_.getFileId(key_b);
  };
  block_store_.adviseAccessPattern(internal::AccessPattern::SEQUENTIAL);
  table_.forEachEntry(table_entry_proc, compare);
  block_store_.adviseAccessPattern(internal::AccessPattern::RANDOM);
}

void Map::forEachKey(Callables::BytesProcedure procedure) const {
  table_.forEachKey(procedure);
}

void Map::forEachValue(const Bytes& key,
                       Callables::BytesProcedure procedure) const {
  const auto list_lock = table_.getShared(key);
  if (list_lock.hasList()) {
    list_lock.list()->forEach(procedure, callbacks_.request_blocks);
  }
}

void Map::forEachValue(const Bytes& key,
                       Callables::BytesPredicate predicate) const {
  const auto list_lock = table_.getShared(key);
  if (list_lock.hasList()) {
    list_lock.list()->forEach(predicate, callbacks_.request_blocks);
  }
}

std::map<std::string, std::string> Map::getProperties() const {
  std::map<std::string, std::string> properties;
  properties["major_version"] = std::to_string(MAJOR_VERSION);
  properties["minor_version"] = std::to_string(MINOR_VERSION);
  const auto store_stats = block_store_.getStats().toMap("store");
  const auto table_stats = table_.getStats().toMap("table");
  properties.insert(store_stats.begin(), store_stats.end());
  properties.insert(table_stats.begin(), table_stats.end());
  return properties;
}

std::size_t Map::max_key_size() const { return table_.max_key_size(); }

std::size_t Map::max_value_size() const {
  return options_.block_size - internal::Block::SIZE_OF_VALUE_SIZE_FIELD;
}

void Map::importFromBase64(const boost::filesystem::path& directory,
                           const boost::filesystem::path& file) {
  importFromBase64(directory, file, false);
}

void Map::importFromBase64(const boost::filesystem::path& directory,
                           const boost::filesystem::path& file,
                           bool create_if_missing) {
  importFromBase64(directory, file, create_if_missing, -1);
}

void Map::importFromBase64(const boost::filesystem::path& directory,
                           const boost::filesystem::path& file,
                           bool create_if_missing, std::size_t block_size) {
  Options options;
  options.create_if_missing = create_if_missing;
  if (block_size != static_cast<std::size_t>(-1)) {
    options.block_size = block_size;
  }
  Map map(directory, options);
  map.importFromBase64(file);
}

void Map::initCallbacks() {
  // Thread-safe: yes.
  callbacks_.new_block = [this]() { return block_pool_.allocate(); };

  // Thread-safe: yes.
  callbacks_.commit_block =
      [this](const Bytes& key, const internal::Block& block) {
    return block_store_.put(key, block);
  };

  // Thread-safe: yes.
  callbacks_.replace_blocks =
      [this](const std::vector<internal::BlockWithId>& blocks) {
    block_store_.replace(blocks);
  };

  // Thread-safe: yes.
  callbacks_.request_blocks =
      [this](std::vector<internal::BlockWithId>* blocks,
             internal::Arena* arena) { block_store_.get(blocks, arena); };
}

std::size_t Map::remove(const Bytes& key, Callables::BytesPredicate predicate,
                        bool apply_to_all) {
  std::size_t num_deleted = 0;
  auto iter = getMutable(key);
  for (iter.seekToFirst(); iter.hasValue(); iter.next()) {
    if (predicate(iter.getValue())) {
      iter.markAsDeleted();
      ++num_deleted;
      iter.next();
      break;
    }
  }
  if (apply_to_all) {
    for (; iter.hasValue(); iter.next()) {
      if (predicate(iter.getValue())) {
        iter.markAsDeleted();
        ++num_deleted;
      }
    }
  }
  return num_deleted;
}

std::size_t Map::replace(const Bytes& key, Callables::BytesFunction function,
                         bool apply_to_all) {
  std::vector<std::string> updated_values;
  auto iter = getMutable(key);
  for (iter.seekToFirst(); iter.hasValue(); iter.next()) {
    const auto updated_value = function(iter.getValue());
    if (!updated_value.empty()) {
      updated_values.emplace_back(updated_value);
      iter.markAsDeleted();
      iter.next();
      break;
    }
  }
  if (apply_to_all) {
    for (; iter.hasValue(); iter.next()) {
      const auto updated_value = function(iter.getValue());
      if (!updated_value.empty()) {
        updated_values.emplace_back(updated_value);
        iter.markAsDeleted();
      }
    }
  }
  if (!updated_values.empty()) {
    auto list_lock = iter.releaseListLock();
    for (const auto& value : updated_values) {
      list_lock.list()->add(key, value, callbacks_.new_block,
                            callbacks_.commit_block);
    }
  }
  return updated_values.size();
}

void Map::importFromBase64(const boost::filesystem::path& file) {
  std::ifstream ifs(file.string());
  internal::check(ifs, "Cannot open '%s'.", file.c_str());

  std::string key_as_base64;
  std::string key_as_binary;
  std::string value_as_base64;
  std::string value_as_binary;
  if (ifs >> key_as_base64) {
    internal::Base64::decode(key_as_base64, &key_as_binary);
    while (ifs) {
      switch (ifs.peek()) {
        case '\n':
        case '\r':
          ifs >> key_as_base64;
          internal::Base64::decode(key_as_base64, &key_as_binary);
          break;
        case '\f':
        case '\t':
        case '\v':
        case ' ':
          ifs.ignore();
          break;
        default:
          ifs >> value_as_base64;
          internal::Base64::decode(value_as_base64, &value_as_binary);
          put(key_as_binary, value_as_binary);
      }
    }
  }
}

void optimize(const boost::filesystem::path& from,
              const boost::filesystem::path& to) {
  optimize(from, to, -1);
}

void optimize(const boost::filesystem::path& from,
              const boost::filesystem::path& to, std::size_t new_block_size) {
  optimize(from, to, Callables::BytesCompare(), new_block_size);
}

void optimize(const boost::filesystem::path& from,
              const boost::filesystem::path& to,
              Callables::BytesCompare compare) {
  optimize(from, to, compare, -1);
}

void optimize(const boost::filesystem::path& from,
              const boost::filesystem::path& to,
              Callables::BytesCompare compare, std::size_t new_block_size) {
  Map map_in(from, Options());

  Options options;
  options.error_if_exists = true;
  options.create_if_missing = true;
  options.block_size = std::stoul(map_in.getProperties()["store.block_size"]);
  if (new_block_size != static_cast<std::size_t>(-1)) {
    options.block_size = new_block_size;
  }
  Map map_out(to, options);

  if (compare) {
    map_in.forEachEntry([&](const Bytes& key, Map::ConstListIter&& iter) {
      std::vector<std::string> values;
      values.reserve(iter.num_values());
      for (iter.seekToFirst(); iter.hasValue(); iter.next()) {
        values.push_back(iter.getValue().toString());
      }
      std::sort(values.begin(), values.end(), compare);
      for (const auto& value : values) {
        map_out.put(key, value);
      }
    });
  } else {
    map_in.forEachEntry([&](const Bytes& key, Map::ConstListIter&& iter) {
      for (iter.seekToFirst(); iter.hasValue(); iter.next()) {
        map_out.put(key, iter.getValue());
      }
    });
  }
}

void exportToBase64(const boost::filesystem::path& directory,
                    const boost::filesystem::path& file) {
  Map map(directory, Options());
  std::ofstream ofs(file.string());
  internal::check(ofs, "Cannot create '%s'.", file.c_str());

  std::string key_as_base64;
  std::string value_as_base64;
  map.forEachEntry([&](const Bytes& key, Map::ConstListIter&& iter) {
    MT_ASSERT_NE(iter.num_values(), 0);
    iter.seekToFirst();
    if (iter.hasValue()) {
      internal::Base64::encode(key, &key_as_base64);
      internal::Base64::encode(iter.getValue(), &value_as_base64);
      ofs << key_as_base64 << ' ' << value_as_base64;
      iter.next();
    }
    while (iter.hasValue()) {
      internal::Base64::encode(iter.getValue(), &value_as_base64);
      ofs << ' ' << value_as_base64;
      iter.next();
    }
    ofs << '\n';
  });
}

}  // namespace multimap
