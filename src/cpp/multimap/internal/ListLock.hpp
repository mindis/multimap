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

#ifndef MULTIMAP_INCLUDE_INTERNAL_LIST_LOCK_HPP
#define MULTIMAP_INCLUDE_INTERNAL_LIST_LOCK_HPP

#include "multimap/internal/List.hpp"

namespace multimap {
namespace internal {

// Wrapper for class List that provides RAII-like locking for shared ownership.
class SharedListLock {
public:
  typedef List::Iter<true> ListIterator;

  SharedListLock() = default;

  explicit SharedListLock(List& list) : list_(&list) { list_->lock_shared(); }

  ~SharedListLock() {
    if (list_) {
      list_->unlock_shared();
    }
  }

  SharedListLock(const SharedListLock&) = delete;
  SharedListLock& operator=(const SharedListLock&) = delete;

  SharedListLock(SharedListLock&& other) : list_(other.list_) {
    other.list_ = nullptr;
  }

  SharedListLock& operator=(SharedListLock&& other) {
    if (list_) {
      list_->unlock_shared();
    }
    list_ = other.list_;
    other.list_ = nullptr;
    return *this;
  }

  bool hasList() const { return list_ != nullptr; }

  const List* list() const { return list_; }

private:
  List* list_ = nullptr;
};

// Wrapper for class List that provides RAII-like locking for unique ownership.
class UniqueListLock {
public:
  typedef List::Iter<false> ListIterator;

  UniqueListLock() = default;

  explicit UniqueListLock(List& list) : list_(&list) { list_->lock(); }

  ~UniqueListLock() {
    if (list_) {
      list_->unlock();
    }
  }

  UniqueListLock(const UniqueListLock&) = delete;
  UniqueListLock& operator=(const UniqueListLock&) = delete;

  UniqueListLock(UniqueListLock&& other) : list_(other.list_) {
    other.list_ = nullptr;
  }

  UniqueListLock& operator=(UniqueListLock&& other) {
    if (list_) {
      list_->unlock_shared();
    }
    list_ = other.list_;
    other.list_ = nullptr;
    return *this;
  }

  bool hasList() const { return list_ != nullptr; }

  List* list() const { return list_; }

private:
  List* list_ = nullptr;
};

} // namespace internal
} // namespace multimap

#endif // MULTIMAP_INCLUDE_INTERNAL_LIST_LOCK_HPP
