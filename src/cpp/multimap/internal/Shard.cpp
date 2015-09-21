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

#include "multimap/internal/Shard.hpp"

#include <cctype>
#include <fstream>
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem/operations.hpp>
#include "multimap/internal/thirdparty/mt.hpp"

namespace multimap {
namespace internal {

namespace {

const char* TABLE_FILE_SUFFIX = ".table";

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

Shard::Stats readStatsFromFile(const boost::filesystem::path& file) {
  std::ifstream ifs(file.string());
  mt::check(ifs, "Could not open '%s'.", file.c_str());

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
  mt::check(!actual_checksum_str.empty(),
            "Properties file '%s' is missing checksum.", file.c_str());
  const auto actual_checksum = std::stoul(actual_checksum_str);

  properties.erase("checksum");
  const auto expected_checksum = mt::crc32(join(properties));
  mt::check(actual_checksum == expected_checksum,
            "Metadata in '%s' has wrong checksum.", file.c_str());
  //  return properties;
  return Shard::Stats();  // TODO
}

}  // namespace

void Shard::Stats::combine(const Stats& other) {
  store.combine(other.store);
  table.combine(other.table);
}

std::map<std::string, std::string> Shard::Stats::toMap() const {
  std::map<std::string, std::string> map;
  const auto store_map = store.toMap("store");
  const auto table_map = table.toMap("table");
  map.insert(store_map.begin(), store_map.end());
  map.insert(table_map.begin(), table_map.end());
  return map;
}

Shard::~Shard() {
  try {
    table_.flushAllListsOrThrowIfLocked();
  } catch (...) {
    std::terminate();
    // The destructor has the precondition that no list is still locked.
  }
}

Shard::Shard(const boost::filesystem::path& prefix) { open(prefix); }

Shard::Shard(const boost::filesystem::path& prefix, std::size_t block_size) {
  create(prefix, block_size);
}

void Shard::open(const boost::filesystem::path& prefix) {
  store_.open(prefix);
  table_.open(prefix.string() + TABLE_FILE_SUFFIX);
  block_arena_.init(store_.block_size());
  initCallbacks();
  table_.set_commit_block_callback(callbacks_.commit_block);
}

void Shard::create(const boost::filesystem::path& prefix,
                   std::size_t block_size) {
  store_.create(prefix, block_size);
  table_.open(prefix.string() + TABLE_FILE_SUFFIX);
  block_arena_.init(store_.block_size());
  initCallbacks();
  table_.set_commit_block_callback(callbacks_.commit_block);
}

void Shard::put(const Bytes& key, const Bytes& value) {
  table_.getUniqueOrCreate(key).list()->add(value, callbacks_.new_block,
                                            callbacks_.commit_block);
}

Shard::ListIterator Shard::get(const Bytes& key) const {
  return ListIterator(table_.getShared(key), callbacks_.request_blocks);
}

Shard::MutableListIterator Shard::getMutable(const Bytes& key) {
  return MutableListIterator(table_.getUnique(key), callbacks_.request_blocks,
                             callbacks_.replace_blocks);
}

bool Shard::contains(const Bytes& key) const {
  const auto list_lock = table_.getShared(key);
  return list_lock.hasList() && !list_lock.clist()->empty();
}

std::size_t Shard::remove(const Bytes& key) {
  std::size_t num_deleted = 0;
  auto list_lock = table_.getUnique(key);
  if (list_lock.hasList()) {
    num_deleted = list_lock.clist()->size();
    list_lock.list()->clear();
  }
  return num_deleted;
}

std::size_t Shard::removeAll(const Bytes& key,
                             Callables::BytesPredicate predicate) {
  return remove(key, predicate, true);
}

std::size_t Shard::removeAllEqual(const Bytes& key, const Bytes& value) {
  return removeAll(key, [&value](const Bytes& current_value) {
    return current_value == value;
  });
}

bool Shard::removeFirst(const Bytes& key, Callables::BytesPredicate predicate) {
  return remove(key, predicate, false);
}

bool Shard::removeFirstEqual(const Bytes& key, const Bytes& value) {
  return removeFirst(key, [&value](const Bytes& current_value) {
    return current_value == value;
  });
}

std::size_t Shard::replaceAll(const Bytes& key,
                              Callables::BytesFunction function) {
  return replace(key, function, true);
}

std::size_t Shard::replaceAllEqual(const Bytes& key, const Bytes& old_value,
                                   const Bytes& new_value) {
  return replaceAll(key, [&old_value, &new_value](const Bytes& current_value) {
    return (current_value == old_value) ? new_value.toString() : std::string();
  });
}

bool Shard::replaceFirst(const Bytes& key, Callables::BytesFunction function) {
  return replace(key, function, false);
}

bool Shard::replaceFirstEqual(const Bytes& key, const Bytes& old_value,
                              const Bytes& new_value) {
  return replaceFirst(key,
                      [&old_value, &new_value](const Bytes& current_value) {
    return (current_value == old_value) ? new_value.toString() : std::string();
  });
}

void Shard::forEachKey(Callables::BytesProcedure procedure) const {
  table_.forEachKey(procedure);
}

void Shard::forEachValue(const Bytes& key,
                         Callables::BytesProcedure procedure) const {
  const auto list_lock = table_.getShared(key);
  if (list_lock.hasList()) {
    list_lock.list()->forEach(procedure, callbacks_.request_blocks);
  }
}

void Shard::forEachValue(const Bytes& key,
                         Callables::BytesPredicate predicate) const {
  const auto list_lock = table_.getShared(key);
  if (list_lock.hasList()) {
    list_lock.list()->forEach(predicate, callbacks_.request_blocks);
  }
}

void Shard::forEachEntry(EntryProcedure procedure) const {
  store_.adviseAccessPattern(Store::AccessPattern::SEQUENTIAL);
  table_.forEachEntry(
      [procedure, this](const Bytes& key, SharedListLock&& list_lock) {
        procedure(
            key, ListIterator(std::move(list_lock), callbacks_.request_blocks));
      });
  store_.adviseAccessPattern(Store::AccessPattern::RANDOM);
}

std::size_t Shard::max_key_size() const { return table_.max_key_size(); }

std::size_t Shard::max_value_size() const {
  return Block::max_value_size(store_.block_size());
}

Shard::Stats Shard::getStats() const {
  return {store_.getStats(), table_.getStats()};
}

void Shard::flush() { table_.flushAllUnlockedLists(); }

void Shard::optimize() { optimize(Callables::BytesCompare(), -1); }

void Shard::optimize(std::size_t new_block_size) {
  optimize(Callables::BytesCompare(), new_block_size);
}

void Shard::optimize(Callables::BytesCompare compare) { optimize(compare, -1); }

void Shard::optimize(Callables::BytesCompare compare,
                     std::size_t new_block_size) {
  MT_ASSERT_TRUE(false);
}

void Shard::initCallbacks() {
  // Thread-safe: yes.
  callbacks_.new_block = [this]() { return block_arena_.allocate(); };

  // Thread-safe: yes.
  callbacks_.commit_block =
      [this](const Block& block) { return store_.append(block); };

  // Thread-safe: yes.
  callbacks_.replace_blocks = [this](const std::vector<BlockWithId>& blocks) {
    for (const auto& block : blocks) {
      if (block.ignore) continue;
      store_.write(block.id, block);
    }
  };

  // Thread-safe: yes.
  callbacks_.request_blocks =
      [this](std::vector<BlockWithId>* blocks, Arena* arena) {
    MT_REQUIRE_NOT_NULL(blocks);

    for (auto& block : *blocks) {
      if (block.ignore) continue;
      store_.read(block.id, &block, arena);
    }
  };
}

std::size_t Shard::remove(const Bytes& key, Callables::BytesPredicate predicate,
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

std::size_t Shard::replace(const Bytes& key, Callables::BytesFunction function,
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
      list_lock.list()->add(value, callbacks_.new_block,
                            callbacks_.commit_block);
    }
  }
  return updated_values.size();
}

}  // namespace internal
}  // namespace multimap
