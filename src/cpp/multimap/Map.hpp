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

#ifndef MULTIMAP_MAP_HPP
#define MULTIMAP_MAP_HPP

#include <map>
#include <string>
#include <boost/filesystem/path.hpp>
#include "multimap/Callables.hpp"
#include "multimap/Options.hpp"
#include "multimap/Iterator.hpp"
#include "multimap/internal/AutoCloseFile.hpp"
#include "multimap/internal/BlockPool.hpp"
#include "multimap/internal/Callbacks.hpp"
#include "multimap/internal/Store.hpp"
#include "multimap/internal/Table.hpp"
#include "multimap/internal/System.hpp"

namespace multimap {

static const std::size_t kMajorVersion = 0;
static const std::size_t kMinorVersion = 2;

class Map {
 public:
  typedef Iterator<false> Iter;
  typedef Iterator<true> ConstIter;

  Map();

  ~Map();

  Map(const boost::filesystem::path& directory, const Options& options);

  void Open(const boost::filesystem::path& directory, const Options& options);

  void Put(const Bytes& key, const Bytes& value);

  ConstIter Get(const Bytes& key) const;

  Iter GetMutable(const Bytes& key);

  bool Contains(const Bytes& key) const;

  std::size_t Delete(const Bytes& key);

  std::size_t DeleteAll(const Bytes& key, Callables::Predicate predicate);

  std::size_t DeleteAllEqual(const Bytes& key, const Bytes& value);

  bool DeleteFirst(const Bytes& key, Callables::Predicate predicate);

  bool DeleteFirstEqual(const Bytes& key, const Bytes& value);

  std::size_t ReplaceAll(const Bytes& key, Callables::Function function);

  std::size_t ReplaceAllEqual(const Bytes& key, const Bytes& old_value,
                              const Bytes& new_value);

  bool ReplaceFirst(const Bytes& key, Callables::Function function);

  bool ReplaceFirstEqual(const Bytes& key, const Bytes& old_value,
                         const Bytes& new_value);

  void ForEachKey(Callables::Procedure procedure) const;

  void ForEachValue(const Bytes& key, Callables::Procedure procedure) const;

  void ForEachValue(const Bytes& key, Callables::Predicate predicate) const;

  std::map<std::string, std::string> GetProperties() const;

  std::size_t max_key_size() const;

  std::size_t max_value_size() const;

 private:
  std::size_t Delete(const Bytes& key, Callables::Predicate pred, bool all);

  std::size_t Replace(const Bytes& key, Callables::Function func, bool all);

  void InitCallbacks();

  // Members may access each other in destructor calls,
  // thus the order of declaration matters - don't reorder.
  internal::System::DirectoryLockGuard directory_lock_guard_;
  internal::Callbacks callbacks_;
  internal::BlockPool block_pool_;
  internal::Store store_;
  internal::Table table_;
  Options options_;
};

void Optimize(const boost::filesystem::path& from,
              const boost::filesystem::path& to);

void Optimize(const boost::filesystem::path& from,
              const boost::filesystem::path& to, std::size_t new_block_size);

void Optimize(const boost::filesystem::path& from,
              const boost::filesystem::path& to,
              const Callables::Compare& compare);

void Optimize(const boost::filesystem::path& from,
              const boost::filesystem::path& to,
              const Callables::Compare& compare, std::size_t new_block_size);

void Import(const boost::filesystem::path& directory,
            const boost::filesystem::path& file);

void Import(const boost::filesystem::path& directory,
            const boost::filesystem::path& file, bool create_if_missing);

void Import(const boost::filesystem::path& directory,
            const boost::filesystem::path& file, bool create_if_missing,
            std::size_t block_size);

void Export(const boost::filesystem::path& directory,
            const boost::filesystem::path& file);

}  // namespace multimap

#endif  // MULTIMAP_MAP_HPP
