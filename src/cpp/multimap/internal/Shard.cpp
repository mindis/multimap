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

std::size_t Shard::Limits::max_key_size() {
  return Table::Limits::max_key_size();
}

std::size_t Shard::Limits::max_value_size() {
  return Store::Limits::max_value_size();
}

Shard::Stats& Shard::Stats::combine(const Stats& other) {
  store.combine(other.store);
  table.combine(other.table);
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

Shard::Shard(const boost::filesystem::path& prefix, const Options& options) {
  //  store_.open(prefix.string() + STORE_FILE_SUFFIX, block_size);
//  table_.open(prefix.string() + TABLE_FILE_SUFFIX);
  Table::Options table_opts;
  table_opts.block_size = options.block_size;
  table_opts.buffer_size = options.buffer_size;
  table_opts.create_if_missing = options.create_if_missing;
  table_opts.error_if_exists = options.error_if_exists;
  table_.reset(new Table(prefix, table_opts));
  // TODO Check if arena_(large_chunk_size) makes any difference.
  prefix_ = prefix;
}

void Shard::put(const Bytes& key, const Bytes& value) {
  table_->getUniqueOrCreate(key).add(value);
}

SharedListIterator Shard::getShared(const Bytes& key) const {
  return SharedListIterator(table_->getShared(key));
}

UniqueListIterator Shard::getUnique(const Bytes& key) {
  return UniqueListIterator(table_->getUnique(key));
}

bool Shard::contains(const Bytes& key) const {
  const auto list = table_->getShared(key);
  return list.isNull() ? false : !list.empty();
}

std::size_t Shard::remove(const Bytes& key) {
  auto list = table_->getUnique(key);
  return list.isNull() ? 0 : list.clear();
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
  table_->forEachKey(action);
}

void Shard::forEachValue(const Bytes& key, Callables::Procedure action) const {
  auto iter = SharedListIterator(table_->getShared(key));
  while (iter.hasNext()) {
    action(iter.next());
  }
}

void Shard::forEachValue(const Bytes& key, Callables::Predicate action) const {
  auto iter = SharedListIterator(table_->getShared(key));
  while (iter.hasNext()) {
    if (!action(iter.next())) {
      break;
    }
  }
}

void Shard::forEachEntry(Callables::Procedure2 action) const {
  table_->forEachEntry([action, this](const Bytes& key, SharedList&& list) {
    action(key, SharedListIterator(std::move(list)));
  });
}

Shard::Stats Shard::getStats() const {
  Stats stats;
//  stats.store = store_.getStats();
  stats.table = table_->getStats();
  return stats;
}

std::size_t Shard::remove(const Bytes& key, Callables::Predicate predicate,
                          bool exit_on_first_success) {
  std::size_t num_removed = 0;
  auto iter = getUnique(key);
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
  UniqueListIterator iter = table_->getUnique(key);
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
  auto list = iter.release();
  for (const auto& value : replaced_values) {
    list.add(value);
  }
  return replaced_values.size();
}

} // namespace internal
} // namespace multimap
