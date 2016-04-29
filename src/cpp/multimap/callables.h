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
// Documentation:  https://multimap.io/cppreference/#callableshpp
// -----------------------------------------------------------------------------

#ifndef MULTIMAP_CALLABLES_H_
#define MULTIMAP_CALLABLES_H_

#include <functional>
#include "multimap/Arena.h"
#include "multimap/Iterator.h"
#include "multimap/Slice.h"

namespace multimap {

typedef std::function<bool(const Slice&)> Predicate;

typedef std::function<void(const Slice&)> Procedure;

typedef std::function<Bytes(const Slice&)> Function;

typedef std::function<Slice(const Slice&, Arena*)> Function2;

typedef std::function<bool(const Slice&, const Slice&)> Compare;

typedef std::function<void(const Slice&, Iterator*)> BinaryProcedure;

}  // namespace multimap

#endif  // MULTIMAP_CALLABLES_H_
