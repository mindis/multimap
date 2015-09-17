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

#ifndef MULTIMAP_INCLUDE_CALLABLES_HPP
#define MULTIMAP_INCLUDE_CALLABLES_HPP

#include <functional>
#include <string>

namespace multimap {

class Bytes;
template <bool IsConst> class Iterator;

// This class contains typedefs which define signatures of various callable
// types. See std::function for more details on how to create such objects
// from lambda expressions, class methods, or free functions.
struct Callables {
  typedef std::function<bool(const Bytes&, const Bytes&)> BytesCompare;
  // Types implementing this interface can process two values and return a
  // boolean. Such functions determine the less than order of the given values
  // according to the Compare concept.
  // See http://en.cppreference.com/w/cpp/concept/Compare

  typedef std::function<std::string(const Bytes&)> BytesFunction;
  // Types implementing this interface can process a value and return a new one.
  // Functions map an input value to an output value. An empty or no result can
  // be signaled returning an empty string. std::string is used here as a
  // convenient byte buffer that may contain arbitrary bytes.

  typedef std::function<bool(const Bytes&)> BytesPredicate;
  // Types implementing this interface can process a value and return a boolean.
  // Predicates check a value for certain property and thus, depending on the
  // outcome, can be used to control the path of execution.

  typedef std::function<void(const Bytes&)> BytesProcedure;
  // Types implementing this interface can process a value, but do not return a
  // result. However, since objects of this type may have state, a procedure
  // can be used to collect information about the processed data, and thus
  // returning a result indirectly.

  typedef std::function<void(const Bytes&, Iterator<true>&&)> EntryProcedure;
};

}  // namespace multimap

#endif  // MULTIMAP_INCLUDE_CALLABLES_HPP
