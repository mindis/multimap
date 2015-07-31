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

#ifndef MULTIMAP_CALLABLES_HPP
#define MULTIMAP_CALLABLES_HPP

#include <functional>
#include "multimap/Bytes.hpp"

namespace multimap {

struct Callables {
  typedef std::function<void(const Bytes&)> Procedure;
  typedef std::function<bool(const Bytes&)> Predicate;
  typedef std::function<std::string(const Bytes&)> Function;
  typedef std::function<bool(const Bytes&, const Bytes&)> Compare;
};

}  // namespace multimap

#endif  // MULTIMAP_CALLABLES_HPP
