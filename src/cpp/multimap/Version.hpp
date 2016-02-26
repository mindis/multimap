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

#ifndef MULTIMAP_VERSION_HPP_INCLUDED
#define MULTIMAP_VERSION_HPP_INCLUDED

namespace multimap {

struct Version {
  static const int MAJOR = 0;
  static const int MINOR = 5;
  static const int PATCH = 0;

  static void checkCompatibility(int extern_major, int extern_minor);

  static bool isCompatible(int extern_major, int extern_minor,
                           int library_major = MAJOR,
                           int library_minor = MINOR);

  Version() = delete;
};

}  // namespace multimap

#endif  // MULTIMAP_VERSION_HPP_INCLUDED
