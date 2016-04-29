// This file is part of Multimap.  http://multimap.io
//
// Copyright (C) 2015-2016  Martin Trenkmann
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

// -----------------------------------------------------------------------------
// Documentation:  https://multimap.io/cppreference/#iteratorhpp
// -----------------------------------------------------------------------------

#ifndef MULTIMAP_ITERATOR_H_
#define MULTIMAP_ITERATOR_H_

#include <iterator>
#include <memory>
#include "multimap/Slice.h"

namespace multimap {

class Iterator {
 public:
  static std::unique_ptr<Iterator> newEmptyInstance();

  virtual ~Iterator() = default;

  virtual size_t available() const = 0;

  virtual bool hasNext() const = 0;

  virtual Slice next() = 0;

  virtual Slice peekNext() = 0;
};

template <typename InputIter>
class RangeIterator : public Iterator {
 public:
  RangeIterator(InputIter begin, InputIter end) : begin_(begin), end_(end) {}

  size_t available() const override { return std::distance(begin_, end_); }

  bool hasNext() const override { return begin_ != end_; }

  Slice next() override { return *begin_++; }

  Slice peekNext() override { return *begin_; }

 private:
  InputIter begin_;
  InputIter end_;
};

template <typename InputIter>
RangeIterator<InputIter> makeRangeIterator(InputIter begin, InputIter end) {
  return RangeIterator<InputIter>(begin, end);
}

}  // namespace multimap

#endif  // MULTIMAP_ITERATOR_H_
