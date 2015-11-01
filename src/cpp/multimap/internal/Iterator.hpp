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

#ifndef MULTIMAP_INTERNAL_ITERATOR_HPP_INCLUDED
#define MULTIMAP_INTERNAL_ITERATOR_HPP_INCLUDED

#include <cstdint>
#include "multimap/internal/Callbacks.hpp"
#include "multimap/internal/ListLock.hpp"

namespace multimap {
namespace internal {

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
//
// The interface is inspired by Qt's QListIterator and Java's Iterator classes.
// http://doc.qt.io/qt-5/qlistiterator.html
// http://docs.oracle.com/javase/7/docs/api/java/util/Iterator.html
template <typename ListLock> class Iterator {
public:
  Iterator() = default;

  Iterator(ListLock&& list_lock,
           const Callbacks::RequestBlocks& request_blocks_callback);
  // Specialized only for ListLock == SharedListLock.

  Iterator(ListLock&& list_lock,
           const Callbacks::RequestBlocks& request_blocks_callback,
           const Callbacks::ReplaceBlocks& replace_blocks_callback);
  // Specialized only for ListLock == UniqueListLock.

  Iterator(Iterator&&) = default;
  Iterator& operator=(Iterator&&) = default;

  std::size_t available() const { return list_iter_.available(); }

  //  bool findNext(Callables::Predicate predicate) {
  //    while (hasNext()) {
  //      if (predicate(next())) return true;
  //    }
  //    return false;
  //  }

  //  bool findNextEqual(const Bytes& value) {
  //    while (hasNext()) {
  //      if (next() == value) return true;
  //    }
  //    return false;
  //  }

  bool hasNext() { return list_iter_.hasNext(); }

  Bytes next() { return list_iter_.next(); }

  Bytes peekNext() const { return list_iter_.peekNext(); }

  void remove();
  // Specialized only for ListLock == UniqueListLock.

  //  void seekToFirst();
  //  // Initializes the iterator to point to the first value, if any. This
  //  process
  //  // will trigger disk IO if necessary. The method can also be used to seek
  //  // back to the beginning of the list at the end of an iteration.

  //  void seekTo(const Bytes& target);
  //  // Initializes the iterator to point to the first value in the list that
  //  is
  //  // equal to target, if any. This process will trigger disk IO if
  //  necessary.

  //  void seekTo(BytesPredicate predicate);
  //  // Initializes the iterator to point to the first value for which
  //  predicate
  //  // yields true, if any. This process will trigger disk IO if necessary.

  //  bool hasValue() const;
  //  // Tells whether the iterator points to a value. If the result is true,
  //  the
  //  // iterator may be dereferenced via getValue().

  //  Bytes getValue() const;
  //  // Returns the current value. The returned Bytes object wraps a pointer to
  //  // data that is managed by the iterator. Hence, this pointer is only valid
  //  as
  //  // long as the iterator does not move forward. Therefore, the value should
  //  // only be used to immediately parse information or some user-defined
  //  object
  //  // out of it. If an independent deep copy is needed you can call
  //  // Bytes::toString().
  //  // Preconditions:
  //  //   * hasValue() == true

  //  void markAsDeleted();
  //  // Marks the value the iterator currently points to as deleted.
  //  // Preconditions:
  //  //   * hasValue() == true
  //  // Postconditions:
  //  //   * hasValue() == false

  //  void next();
  //  // Moves the iterator to the next value, if any.

  //  ListLock<IsConst> releaseListLock();

  //  std::size_t num_values() const;
  // Returns the total number of values to iterate. This number does not change
  // when the iterator moves forward. The method may be called at any time,
  // even if seekToFirst() or one of its friends have not been called.

private:
  typename ListLock::ListIterator list_iter_;
  ListLock list_lock_;
};

template <>
inline Iterator<SharedListLock>::Iterator(
    SharedListLock&& list_lock,
    const Callbacks::RequestBlocks& request_blocks_callback)
    : list_lock_(std::move(list_lock)) {
  if (list_lock_.hasList()) {
    list_iter_ = list_lock_.list()->const_iterator(request_blocks_callback);
  }
}

template <>
inline Iterator<UniqueListLock>::Iterator(
    UniqueListLock&& list_lock,
    const Callbacks::RequestBlocks& request_blocks_callback,
    const Callbacks::ReplaceBlocks& replace_blocks_callback)
    : list_lock_(std::move(list_lock)) {
  if (list_lock_.hasList()) {
    list_iter_ = list_lock_.list()->iterator(request_blocks_callback,
                                             replace_blocks_callback);
  }
}

template <> inline void Iterator<UniqueListLock>::remove() {
  list_iter_.remove();
}

} // namespace internal
} // namespace multimap

#endif // MULTIMAP_INTERNAL_ITERATOR_HPP_INCLUDED
