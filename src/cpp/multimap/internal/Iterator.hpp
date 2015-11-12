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

#include "multimap/internal/List.hpp"
#include "multimap/internal/Store.hpp"

namespace multimap {
namespace internal {

// -----------------------------------------------------------------------------
// class SharedListIterator

class SharedListIterator {
public:
  SharedListIterator() = default;

  SharedListIterator(SharedListPointer&& list, const Store& store)
      : list_(std::move(list)) {
    if (list_) {
      iter_ = list_->iterator(store);
    }
  }

  SharedListIterator(SharedListIterator&&) = default;
  SharedListIterator& operator=(SharedListIterator&&) = default;

  std::size_t available() const { return iter_.available(); }

  bool hasNext() { return iter_.hasNext(); }

  Bytes next() { return iter_.next(); }

  Bytes peekNext() { return iter_.peekNext(); }

private:
  List::Iterator iter_;
  SharedListPointer list_;
};

// -----------------------------------------------------------------------------
// class UniqueListIterator

class UniqueListIterator {
public:
  UniqueListIterator() = default;

  UniqueListIterator(UniqueListPointer&& list, Store* store)
      : list_(std::move(list)) {
    if (list_) {
      iter_ = list_->iterator(store);
    }
  }

  UniqueListIterator(UniqueListIterator&&) = default;
  UniqueListIterator& operator=(UniqueListIterator&&) = default;

  std::size_t available() const { return iter_.available(); }

  bool hasNext() { return iter_.hasNext(); }

  Bytes next() { return iter_.next(); }

  Bytes peekNext() { return iter_.peekNext(); }

  void remove() { iter_.remove(); }

private:
  List::MutableIterator iter_;
  UniqueListPointer list_;
};

} // namespace internal
} // namespace multimap

#endif // MULTIMAP_INTERNAL_ITERATOR_HPP_INCLUDED
