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

#include <boost/filesystem/operations.hpp>

namespace multimap {
namespace internal {

namespace {

const char* STORE_FILE_SUFFIX = ".store";
const char* TABLE_FILE_SUFFIX = ".table";

} // namespace

Shard::Stats& Shard::Stats::combine(const Stats& other) {
  store.summarize(other.store);
  table.summarize(other.table);
  return *this;
}

Shard::Stats Shard::Stats::combine(const Stats& a, const Stats& b) {
  return Stats(a).combine(b);
}

Shard::Stats Shard::Stats::fromProperties(const mt::Properties& properties) {
  Stats stats;
  stats.store = Store::Stats::fromProperties(properties, "store");
  stats.table = Table::Stats::fromProperties(properties, "table");
  return stats;
}

mt::Properties Shard::Stats::toProperties() const {
  mt::Properties properties;
  const auto store_properties = store.toProperties("store");
  const auto table_properties = table.toProperties("table");
  properties.insert(store_properties.begin(), store_properties.end());
  properties.insert(table_properties.begin(), table_properties.end());
  return properties;
}

Shard::~Shard() {
  if (isOpen()) {
    close();
  }
}

Shard::Shard(const boost::filesystem::path& prefix) { open(prefix); }

Shard::Shard(const boost::filesystem::path& prefix, std::size_t block_size) {
  open(prefix, block_size);
}

void Shard::open(const boost::filesystem::path& prefix) { open(prefix, 0); }

void Shard::open(const boost::filesystem::path& prefix,
                 std::size_t block_size) {
  MT_REQUIRE_FALSE(isOpen());

  store_.open(prefix.string() + STORE_FILE_SUFFIX, block_size);
  table_.open(prefix.string() + TABLE_FILE_SUFFIX);
  // TODO Check if arena_(large_chunk_size) makes any difference.
  prefix_ = prefix;
  initCallbacks();

  MT_REQUIRE_TRUE(isOpen());
}

bool Shard::isOpen() const { return table_.isOpen(); }

Shard::Stats Shard::close() {
  MT_REQUIRE_TRUE(isOpen());

  Stats stats;
  stats.table = table_.close(callbacks_.commit_block);
  stats.store = store_.getStats();

  MT_ENSURE_FALSE(isOpen());
  return stats;
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
  return list_lock.hasList() && !list_lock.list()->empty();
}

std::size_t Shard::remove(const Bytes& key) {
  std::size_t num_removed = 0;
  auto list_lock = table_.getUnique(key);
  if (list_lock.hasList()) {
    num_removed = list_lock.list()->size();
    list_lock.list()->clear();
  }
  return num_removed;
}

std::size_t Shard::removeAll(const Bytes& key, Callables::Predicate predicate) {
  return remove(key, predicate, false);
}

std::size_t Shard::removeAllEqual(const Bytes& key, const Bytes& value) {
  return removeAll(key, [&value](const Bytes& current_value) {
    return current_value == value;
  });
}

bool Shard::removeFirst(const Bytes& key, Callables::Predicate predicate) {
  return remove(key, predicate, true);
}

bool Shard::removeFirstEqual(const Bytes& key, const Bytes& value) {
  return removeFirst(key, [&value](const Bytes& current_value) {
    return current_value == value;
  });
}

std::size_t Shard::replaceAll(const Bytes& key, Callables::Function function) {
  return replace(key, function, false);
}

std::size_t Shard::replaceAllEqual(const Bytes& key, const Bytes& old_value,
                                   const Bytes& new_value) {
  return replaceAll(key, [&old_value, &new_value](const Bytes& current_value) {
    return (current_value == old_value) ? new_value.toString() : std::string();
  });
}

bool Shard::replaceFirst(const Bytes& key, Callables::Function function) {
  return replace(key, function, true);
}

bool Shard::replaceFirstEqual(const Bytes& key, const Bytes& old_value,
                              const Bytes& new_value) {
  return replaceFirst(key,
                      [&old_value, &new_value](const Bytes& current_value) {
    return (current_value == old_value) ? new_value.toString() : std::string();
  });
}

void Shard::forEachKey(Callables::Procedure action) const {
  table_.forEachKey(action);
}

void Shard::forEachValue(const Bytes& key, Callables::Procedure action) const {
  const auto list_lock = table_.getShared(key);
  if (list_lock.hasList()) {
    auto iter = list_lock.list()->const_iterator(callbacks_.request_blocks);
    while (iter.hasNext()) {
      action(iter.next());
    }
  }
}

void Shard::forEachValue(const Bytes& key, Callables::Predicate action) const {
  const auto list_lock = table_.getShared(key);
  if (list_lock.hasList()) {
    auto iter = list_lock.list()->const_iterator(callbacks_.request_blocks);
    while (iter.hasNext()) {
      if (!action(iter.next())) {
        break;
      }
    }
  }
}

void Shard::forEachEntry(Callables::Procedure2<ListIterator> action) const {
  store_.adviseAccessPattern(Store::AccessPattern::SEQUENTIAL);
  table_.forEachEntry([action, this](const Bytes& key,
                                     SharedListLock&& list_lock) {
    action(key, ListIterator(std::move(list_lock), callbacks_.request_blocks));
  });
  store_.adviseAccessPattern(Store::AccessPattern::RANDOM);
}

std::size_t Shard::max_key_size() const { return table_.max_key_size(); }

std::size_t Shard::max_value_size() const {
  return Block::max_value_size(store_.block_size());
}

Shard::Stats Shard::getStats() const {
  Stats stats;
  stats.store = store_.getStats();
  stats.table = table_.getStats();
  return stats;
}

void Shard::initCallbacks() {
  // Thread-safe: yes.  // TODO Check this.
  callbacks_.new_block = [this]() {
    const auto block_size = store_.block_size();
    return Block(arena_.allocate(block_size), block_size);
  };

  // Thread-safe: yes.
  callbacks_.commit_block =
      [this](const Block& block) { return store_.append(block); };

  // Thread-safe: yes.
  callbacks_.replace_blocks = [this](const std::vector<BlockWithId>& blocks) {
    for (const auto& block : blocks) {
      if (block.ignore)
        continue;
      store_.write(block.id, block);
    }
  };

  // Thread-safe: yes.
  callbacks_.request_blocks =
      [this](std::vector<BlockWithId>* blocks, Arena* arena) {
    MT_REQUIRE_NOT_NULL(blocks);

    for (auto& block : *blocks) {
      if (block.ignore)
        continue;
      store_.read(block.id, &block, arena);
    }
  };
}

std::size_t Shard::remove(const Bytes& key, Callables::Predicate predicate,
                          bool exit_on_first_success) {
  std::size_t num_removed = 0;
  auto iter = getMutable(key);
  while (iter.hasNext()) {
    if (predicate(iter.next())) {
      iter.remove();
      ++num_removed;
      if (exit_on_first_success)
        break;
    }
  }
  return num_removed;
}

std::size_t Shard::replace(const Bytes& key, Callables::Function function,
                           bool exit_on_first_success) {
  std::vector<std::string> replaced_values;
  UniqueListLock list_lock = table_.getUnique(key);
  if (list_lock.hasList()) {
    auto iter = list_lock.list()->iterator(callbacks_.request_blocks,
                                           callbacks_.replace_blocks);
    while (iter.hasNext()) {
      auto replaced_value = function(iter.next());
      if (!replaced_value.empty()) {
        replaced_values.push_back(std::move(replaced_value));
        iter.remove();
        if (exit_on_first_success) {
          break;
        }
      }
    }
    for (const auto& value : replaced_values) {
      list_lock.list()->add(value, callbacks_.new_block, callbacks_.commit_block);
    }
  }
  return replaced_values.size();
}

} // namespace internal
} // namespace multimap
