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
#include "multimap/internal/Store.hpp"
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

  Iterator(ListLock&& list_lock, const Store& store);

  Iterator(ListLock&& list_lock, Store* store);

  Iterator(Iterator&&) = default;
  Iterator& operator=(Iterator&&) = default;

  std::size_t available() const { return list_iter_.available(); }

  bool hasNext() { return list_iter_.hasNext(); }

  Bytes next() { return list_iter_.next(); }

  Bytes peekNext() const { return list_iter_.peekNext(); }

  void remove();
  // Enabled if ListLock is UniqueListLock.

private:
  typename ListLock::ListIterator list_iter_;
  ListLock list_lock_;
};

template <>
inline Iterator<SharedListLock>::Iterator(SharedListLock&& list_lock,
                                          const Store& store)
    : list_lock_(std::move(list_lock)) {
  if (list_lock_.hasList()) {
    list_iter_ = SharedListLock::ListIterator(*list_lock_.list(), store);
  }
}

template <>
inline Iterator<UniqueListLock>::Iterator(UniqueListLock&& list_lock,
                                          Store* store)
    : list_lock_(std::move(list_lock)) {
  if (list_lock_.hasList()) {
    list_iter_ = UniqueListLock::ListIterator(list_lock_.list(), store);
  }
}

template <> inline void Iterator<UniqueListLock>::remove() {
  list_iter_.remove();
}

typedef Iterator<SharedListLock> SharedListIterator;
typedef Iterator<UniqueListLock> UniqueListIterator;

} // namespace internal
} // namespace multimap

#endif // MULTIMAP_INTERNAL_ITERATOR_HPP_INCLUDED
