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

namespace multimap {

namespace {

void checkOptions(const Options& options) {
  mt::check(options.block_size != 0, "options.block_size must be positive");
  mt::check(mt::isPowerOfTwo(options.block_size),
            "options.block_size must be a power of two");
}

internal::Table& getTable(
    const std::vector<std::unique_ptr<internal::Table> >& tables,
    const Bytes& key) {
  MT_REQUIRE_FALSE(tables.empty());
  return *tables[mt::fnv1aHash(key.data(), key.size()) % tables.size()];
}

std::size_t removeValues(
    const std::vector<std::unique_ptr<internal::Table> >& tables,
    const Bytes& key, Callables::Predicate predicate,
    bool exit_on_first_success) {
  std::size_t num_removed = 0;
  auto iter = Map::MutableListIterator(getTable(tables, key).getUnique(key));
  while (iter.hasNext()) {
    if (predicate(iter.next())) {
      iter.remove();
      ++num_removed;
      if (exit_on_first_success) {
        break;
      }
    }
  }
  return num_removed;
}

std::size_t replaceValues(
    const std::vector<std::unique_ptr<internal::Table> >& tables,
    const Bytes& key, Callables::Function function,
    bool exit_on_first_success) {
  std::vector<std::string> replaced_values;
  auto iter = Map::MutableListIterator(getTable(tables, key).getUnique(key));
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

} // namespace

std::size_t Map::Limits::getMaxKeySize() {
  return internal::Table::Limits::getMaxKeySize();
}

std::size_t Map::Limits::getMaxValueSize() {
  return internal::Table::Limits::getMaxValueSize();
}

Map::Map(const boost::filesystem::path& directory, const Options& options) {
  checkOptions(options);
  const auto abs_dir = boost::filesystem::absolute(directory);
  mt::check(boost::filesystem::is_directory(abs_dir),
            "The path '%s' does not refer to a directory.", abs_dir.c_str());

  lock_ = mt::DirectoryLockGuard(abs_dir, internal::getNameOfLockFile());
  const auto id_file = abs_dir / internal::getNameOfIdFile();

  if (boost::filesystem::is_regular_file(id_file)) {
    mt::check(!options.error_if_exists,
              "A Multimap in '%s' does already exist.", abs_dir.c_str());

    const auto id = internal::Id::readFromFile(id_file);
    internal::checkVersion(id.version_major, id.version_minor);
    tables_.resize(id.num_shards);

  } else {
    mt::check(options.create_if_missing, "No Multimap found in '%s'.",
              abs_dir.c_str());

    tables_.resize(mt::nextPrime(options.num_shards));
  }

  internal::Table::Options table_opts;
  table_opts.block_size = options.block_size;
  table_opts.buffer_size = options.buffer_size;
  table_opts.create_if_missing = options.create_if_missing;
  table_opts.error_if_exists = options.error_if_exists;
  table_opts.readonly = options.readonly;

  const auto prefix = abs_dir / internal::getFilePrefix();
  for (std::size_t i = 0; i != tables_.size(); ++i) {
    const auto prefix_i = prefix.string() + '.' + std::to_string(i);
    tables_[i].reset(new internal::Table(prefix_i, table_opts));
  }
}

Map::~Map() {
  if (!tables_.empty()) {
    internal::Id id;
    id.num_shards = tables_.size();
    id.writeToFile(lock_.directory() / internal::getNameOfIdFile());
  }
}

void Map::put(const Bytes& key, const Bytes& value) {
  getTable(tables_, key).getUniqueOrCreate(key).add(value);
}

Map::ListIterator Map::get(const Bytes& key) const {
  return Map::ListIterator(getTable(tables_, key).getShared(key));
}

Map::MutableListIterator Map::getMutable(const Bytes& key) {
  return Map::MutableListIterator(getTable(tables_, key).getUnique(key));
}

bool Map::contains(const Bytes& key) const {
  if (auto list = getTable(tables_, key).getShared(key)) {
    return list.size() != 0;
  }
  return false;
}

std::size_t Map::remove(const Bytes& key) {
  if (auto list = getTable(tables_, key).getUnique(key)) {
    return list.clear();
  }
  return 0;
}

std::size_t Map::removeAll(const Bytes& key, Callables::Predicate predicate) {
  return removeValues(tables_, key, predicate, false);
}

std::size_t Map::removeAllEqual(const Bytes& key, const Bytes& value) {
  return removeAll(key, [&value](const Bytes& current_value) {
    return current_value == value;
  });
}

bool Map::removeFirst(const Bytes& key, Callables::Predicate predicate) {
  return removeValues(tables_, key, predicate, true);
}

bool Map::removeFirstEqual(const Bytes& key, const Bytes& value) {
  return removeFirst(key, [&value](const Bytes& current_value) {
    return current_value == value;
  });
}

std::size_t Map::replaceAll(const Bytes& key, Callables::Function function) {
  return replaceValues(tables_, key, function, false);
}

std::size_t Map::replaceAllEqual(const Bytes& key, const Bytes& old_value,
                                 const Bytes& new_value) {
  return replaceAll(key, [&old_value, &new_value](const Bytes& current_value)
                             -> std::string {
    return (current_value == old_value) ? new_value.toString() : std::string();
  });
}

bool Map::replaceFirst(const Bytes& key, Callables::Function function) {
  return replaceValues(tables_, key, function, true);
}

bool Map::replaceFirstEqual(const Bytes& key, const Bytes& old_value,
                            const Bytes& new_value) {
  return replaceFirst(key,
                      [&old_value, &new_value](const Bytes& current_value) {
    return (current_value == old_value) ? new_value.toString() : std::string();
  });
}

void Map::forEachKey(Callables::Procedure action) const {
  MT_REQUIRE_FALSE(tables_.empty());
  for (const auto& table : tables_) {
    table->forEachKey(action);
  }
}

void Map::forEachValue(const Bytes& key, Callables::Procedure action) const {
  auto iter = ListIterator(getTable(tables_, key).getShared(key));
  while (iter.hasNext()) {
    action(iter.next());
  }
}

void Map::forEachValue(const Bytes& key, Callables::Predicate action) const {
  auto iter = ListIterator(getTable(tables_, key).getShared(key));
  while (iter.hasNext()) {
    if (!action(iter.next())) {
      break;
    }
  }
}

void Map::forEachEntry(Callables::BinaryProcedure action) const {
  MT_REQUIRE_FALSE(tables_.empty());
  for (const auto& table : tables_) {
    table->forEachEntry(
        [action, this](const Bytes& key, internal::SharedList&& list) {
          action(key, ListIterator(std::move(list)));
        });
  }
}

std::vector<internal::Table::Stats> Map::getStats() const {
  std::vector<internal::Table::Stats> stats;
  for (const auto& table : tables_) {
    stats.push_back(table->getStats());
  }
  return stats;
}

// std::map<std::string, std::string> Map::getProperties() const {
//  MT_REQUIRE_FALSE(tables_.empty());

//  internal::Table::Stats stats;
//  for (const auto& table : tables_) {
//    stats.combine(table->getStats());
//  }
//  auto properties = stats.toProperties();
//  properties["num_shards"] = std::to_string(tables_.size());
//  properties["version_major"] = std::to_string(VERSION_MAJOR);
//  properties["version_minor"] = std::to_string(VERSION_MINOR);
//  return properties;
//}

namespace internal {

Id Id::readFromFile(const boost::filesystem::path& file) {
  std::ifstream ifs(file.string());
  mt::check(ifs, "Could not open '%s' for reading", file.c_str());
  Id id;
  ifs.read(reinterpret_cast<char*>(&id), sizeof id);
  mt::check(ifs, "std::ifstream::read() failed()");
  return id;
}

void Id::writeToFile(const boost::filesystem::path& file) const {
  std::ofstream ofs(file.string());
  mt::check(ofs, "Could not create '%s' for writing", file.c_str());
  ofs.write(reinterpret_cast<const char*>(this), sizeof *this);
  mt::check(ofs, "std::ofstream::write() failed");
}

const std::string getFilePrefix() { return "multimap"; }

const std::string getNameOfIdFile() { return getFilePrefix() + ".id"; }

const std::string getNameOfLockFile() { return getFilePrefix() + ".lock"; }

const std::string getNameOfKeysFile(std::size_t index) {
  const auto prefix = getFilePrefix() + '.' + std::to_string(index);
  return Table::getNameOfKeysFile(prefix);
}

const std::string getNameOfStatsFile(std::size_t index) {
  const auto prefix = getFilePrefix() + '.' + std::to_string(index);
  return Table::getNameOfStatsFile(prefix);
}

const std::string getNameOfValuesFile(std::size_t index) {
  const auto prefix = getFilePrefix() + '.' + std::to_string(index);
  return Table::getNameOfValuesFile(prefix);
}

void checkVersion(std::uint32_t id_major, std::uint32_t id_minor) {
  mt::check(id_major == VERSION_MAJOR && id_minor == VERSION_MINOR,
            "Version check failed. The Multimap you are trying to open "
            "was created with version %u.%u of the library. Your "
            "installed version is %u.%u which is not compatible.",
            id_major, id_minor, VERSION_MAJOR, VERSION_MINOR);
}

} // namespace internal
} // namespace multimap
