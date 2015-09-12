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

// This class template implements a forward iterator on a list of values. The
// template parameter decides whether an instantiation can delete values from
// the underlying list while iterating or not. If the actual parameter is true
// a const iterator will be created where the DeleteValue() method is disabled,
// and vice versa.
//
// The iterator supports lazy initialization, which means that no IO operation
// is performed until one of the methods SeekToFirst(), SeekTo(target), or
// SeekTo(predicate) is called. This might be useful in cases where multiple
// iterators have to be requested first to determine in which order they have
// to be processed.
//
// The iterator also owns a lock for the underlying list to synchronize
// concurrent access. There are two types of locks: a reader lock (also called
// shared lock) and a writer lock (also called unique or exclusive lock). The
// former will be owned by a read-only iterator aka Iterator<true>, the latter
// will be owned by a read-write iterator aka Iterator<false>. The lock is
// automatically released when the lifetime of an iterator object ends and its
// destructor is called.
//
// Users normally don't need to include or instantiate this class directly, but
// use the typedefs Map::ListIter and Map::ConstListIter instead.
template <bool IsConst>
class Iterator {
 public:
  // Creates a default instance that has no values to iterate.
  // Postconditions:
  //   * NumValues() == 0
  Iterator() = default;

  Iterator(internal::ListLock<IsConst>&& list_lock,
           const internal::Callbacks::RequestBlocks& request_blocks_callback);

  Iterator(internal::ListLock<IsConst>&& list_lock,
           const internal::Callbacks::RequestBlocks& request_blocks_callback,
           const internal::Callbacks::ReplaceBlocks& replace_blocks_callback);

  Iterator(Iterator&&) = default;
  Iterator& operator=(Iterator&&) = default;

  // Returns the total number of values to iterate. This number does not change
  // when the iterator moves forward. The method may be called at any time,
  // even if SeekToFirst() or one of its friends have not been called.
  std::size_t NumValues() const;

  // Initializes the iterator to point to the first value, if any. This process
  // will trigger disk IO if necessary. The method can also be used to seek
  // back to the beginning of the list at the end of an iteration.
  void SeekToFirst();

  // Initializes the iterator to point to the first value in the list that is
  // equal to target, if any. This process will trigger disk IO if necessary.
  void SeekTo(const Bytes& target);

  // Initializes the iterator to point to the first value for which predicate
  // yields true, if any. This process will trigger disk IO if necessary.
  void SeekTo(Callables::Predicate predicate);

  // Tells whether the iterator points to a value. If the result is true, the
  // iterator may be dereferenced via GetValue().
  bool HasValue() const;

  // Returns the current value. The returned Bytes object wraps a pointer to
  // data that is managed by the iterator. Hence, this pointer is only valid as
  // long as the iterator does not move forward. Therefore, the value should
  // only be used to immediately parse information or some user-defined object
  // out of it. If an independent deep copy is needed you can call
  // Bytes::ToString().
  // Preconditions:
  //   * HasValue() == true
  Bytes GetValue() const;

  // Marks the value the iterator currently points to as deleted.
  // Preconditions:
  //   * HasValue() == true
  // Postconditions:
  //   * HasValue() == false
  void DeleteValue();

  // Moves the iterator to the next value, if any.
  void Next();

  internal::ListLock<IsConst> ReleaseListLock();

 private:
  internal::ListLock<IsConst> list_lock_;
  internal::List::Iter<IsConst> list_iter_;
};

template <>
inline Iterator<true>::Iterator(
    internal::ListLock<true>&& list_lock,
    const internal::Callbacks::RequestBlocks& request_blocks_callback)
    : list_lock_(std::move(list_lock)) {
  if (list_lock_.clist()) {
    list_iter_ = list_lock_.clist()->NewConstIterator(request_blocks_callback);
  }
}

template <>
inline Iterator<false>::Iterator(
    internal::ListLock<false>&& list_lock,
    const internal::Callbacks::RequestBlocks& request_blocks_callback,
    const internal::Callbacks::ReplaceBlocks& replace_blocks_callback)
    : list_lock_(std::move(list_lock)) {
  if (list_lock_.list()) {
    list_iter_ = list_lock_.list()->NewIterator(request_blocks_callback,
                                                replace_blocks_callback);
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
