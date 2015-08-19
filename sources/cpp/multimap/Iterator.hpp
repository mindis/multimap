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

#ifndef MULTIMAP_ITERATOR_HPP
#define MULTIMAP_ITERATOR_HPP

#include <cstdint>
#include "multimap/Callables.hpp"
#include "multimap/internal/Callbacks.hpp"
#include "multimap/internal/ListLock.hpp"

namespace multimap {

template <bool IsConst>
class Iterator {
 public:
  Iterator() = default;

  Iterator(internal::ListLock<IsConst>&& list_lock,
           const internal::Callbacks::RequestBlock& request_block_callback);

  Iterator(internal::ListLock<IsConst>&& list_lock,
           const internal::Callbacks::RequestBlock& request_block_callback,
           const internal::Callbacks::ReplaceBlock& replace_block_callback);

  Iterator(Iterator&&) = default;
  Iterator& operator=(Iterator&&) = default;

  // Returns the total number of the underlying values, even if not HasValue().
  std::size_t NumValues() const;

  // Support for lazy iterator initialization.
  void SeekToFirst();

  void SeekTo(const Bytes& target);

  void SeekTo(Callables::Predicate predicate);

  // Checks whether the iterator actually points to a value.
  bool HasValue() const;

  // Returns a read-only pointer to the data of the current value.
  // Precondition: HasValue()
  Bytes GetValue() const;

  // Marks the current value as deleted.
  // Only enabled for Lock = UniqueLock.
  // Precondition :  HasValue()
  // Postcondition: !HasValue(), NumValues() == NumValues()'Old - 1
  void DeleteValue();

  // Moves the iterator to the next value.
  void Next();

  internal::ListLock<IsConst> ReleaseListLock();

 private:
  internal::ListLock<IsConst> list_lock_;
  internal::List::Iter<IsConst> list_iter_;
};

template <>
inline Iterator<true>::Iterator(
    internal::ListLock<true>&& list_lock,
    const internal::Callbacks::RequestBlock& request_block_callback)
    : list_lock_(std::move(list_lock)) {
  if (list_lock_.clist()) {
    list_iter_ = list_lock_.clist()->NewIterator(request_block_callback);
  }
}

template <>
inline Iterator<false>::Iterator(
    internal::ListLock<false>&& list_lock,
    const internal::Callbacks::RequestBlock& request_block_callback,
    const internal::Callbacks::ReplaceBlock& replace_block_callback)
    : list_lock_(std::move(list_lock)) {
  if (list_lock_.list()) {
    list_iter_ = list_lock_.list()->NewIterator(request_block_callback,
                                                replace_block_callback);
  }
}

template <bool IsConst>
std::size_t Iterator<IsConst>::NumValues() const {
  const auto list = list_lock_.list();
  return list ? list->size() : 0;
}

template <bool IsConst>
void Iterator<IsConst>::SeekToFirst() {
  list_iter_.SeekToFirst();
}

template <bool IsConst>
void Iterator<IsConst>::SeekTo(const Bytes& target) {
  SeekTo([&target](const Bytes& value) { return value == target; });
}

template <bool IsConst>
void Iterator<IsConst>::SeekTo(Callables::Predicate predicate) {
  for (SeekToFirst(); HasValue(); Next()) {
    if (predicate(GetValue())) {
      break;
    }
  }
}

template <bool IsConst>
bool Iterator<IsConst>::HasValue() const {
  return list_iter_.HasValue();
}

template <bool IsConst>
Bytes Iterator<IsConst>::GetValue() const {
  return list_iter_.GetValue();
}

template <>
inline void Iterator<false>::DeleteValue() {
  list_iter_.Delete();
}

template <bool IsConst>
void Iterator<IsConst>::Next() {
  return list_iter_.Next();
}

template <bool IsConst>
internal::ListLock<IsConst> Iterator<IsConst>::ReleaseListLock() {
  return std::move(list_lock_);
}

}  // namespace multimap

#endif  // MULTIMAP_ITERATOR_HPP
