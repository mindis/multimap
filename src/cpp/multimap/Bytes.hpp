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
// Documentation:  http://multimap.io/cppreference/#byteshpp
// -----------------------------------------------------------------------------

#ifndef MULTIMAP_BYTES_HPP_INCLUDED
#define MULTIMAP_BYTES_HPP_INCLUDED

#include <cstdio>
#include <vector>

namespace multimap {

typedef unsigned char byte;
typedef std::vector<byte> Bytes;

const byte* readBytesFromBuffer(const byte* buffer, Bytes* output);

bool readBytesFromStream(std::FILE* stream, Bytes* output);

void writeBytesToStream(std::FILE* stream, const Bytes& input);

}  // namespace multimap

#endif  // MULTIMAP_BYTES_HPP_INCLUDED
