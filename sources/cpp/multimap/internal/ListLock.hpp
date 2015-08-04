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

#ifndef MULTIMAP_INTERNAL_LIST_LOCK_HPP
#define MULTIMAP_INTERNAL_LIST_LOCK_HPP

#include <functional>
#include "multimap/internal/List.hpp"

namespace multimap {
namespace internal {

// ListLock is a wrapper for class List that provides an RAII-style
// locking/unlocking mechanism similar to std::lock_guard.
template <bool IsShared>
class ListLock {
 public:
  ListLock() : list_(nullptr) {}

  ListLock(List* list);
  ListLock(const List& list);

  ListLock(ListLock&& other) : list_(other.list_) { other.list_ = nullptr; }

  ~ListLock() {
    if (list_) {
      Unlock();
    }
  }

  ListLock& operator=(ListLock&& other) {
    if (&other != this) {
      list_ = other.list_;
      other.list_ = nullptr;
    }
    return *this;
  }

  bool has_list() const { return list_ != nullptr; }

  const List* clist() const { return list_; }

  const List* list() const { return list_; }

  List* list();

 private:
  void Lock() { list_->LockShared(); }

  void Unlock() { list_->UnlockShared(); }

  typename std::conditional<IsShared, const List, List>::type* list_;
};

template <>
inline List* ListLock<false>::list() {
  return list_;
}

template <>
inline void ListLock<false>::Lock() {
  list_->LockUnique();
}

template <>
inline void ListLock<false>::Unlock() {
  list_->UnlockUnique();
}

template <>
inline ListLock<false>::ListLock(List* list)
    : list_(list) {
  assert(list_);
  Lock();
}

template <>
inline ListLock<true>::ListLock(const List& list)
    : list_(&list) {
  Lock();
}

typedef ListLock<true> SharedListLock;
typedef ListLock<false> UniqueListLock;

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_LIST_LOCK_HPP
