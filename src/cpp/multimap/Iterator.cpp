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

#include "multimap/Iterator.hpp"

namespace multimap {

namespace {

class EmptyIter : public Iterator {
 public:
  size_t available() const override { return 0; }

  bool hasNext() const override { return false; }

  Range next() override { return Range(); }

  Range peekNext() override { return Range(); }
};

}  // namespace

std::unique_ptr<Iterator> Iterator::newEmptyInstance() {
  return std::unique_ptr<Iterator>(new EmptyIter());
}

}  // namespace multimap
