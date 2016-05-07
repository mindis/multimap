// This file is part of the MT library.
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

#ifndef MT_VARINT_H_
#define MT_VARINT_H_

#include <istream>
#include <ostream>
#include "mt/common.h"

namespace mt {

// TODO(mtrenkmann): Explain I/O policy wrt using begin/end vs. buffer.

static const int MAX_VARINT32_BYTES = 5;
static const int MAX_VARINT64_BYTES = 10;

// -----------------------------------------------------------------------------
// Unsigned integer buffer I/O
// -----------------------------------------------------------------------------

size_t readVarint32FromBuffer(const byte* buffer, uint32_t* value);

size_t readVarint32FromBuffer(const byte* begin, const byte* end,
                              uint32_t* value);

size_t readVarint64FromBuffer(const byte* buffer, uint64_t* value);
// Reads a varint from an unbounded buffer (no end pointer given).
// The caller has to ensure that buffer points to a valid varint encoding.
// Returns a pointer past the last byte read.

size_t readVarint64FromBuffer(const byte* begin, const byte* end,
                              uint64_t* value);

size_t writeVarint32ToBuffer(uint32_t value, byte* begin, byte* end);
// Returns the number of bytes written if the value could be encoded
// successfully, or zero if there was not enough space in the buffer.

size_t writeVarint64ToBuffer(uint64_t value, byte* begin, byte* end);
// Returns the number of bytes written if the value could be encoded
// successfully, or zero if there was not enough space in the buffer.

// -----------------------------------------------------------------------------
// Unsigned integer file stream I/O
// -----------------------------------------------------------------------------

bool readVarint32FromStream(std::istream* stream, uint32_t* value);

bool readVarint64FromStream(std::istream* stream, uint64_t* value);

void writeVarint32ToStream(uint32_t value, std::ostream* stream);

void writeVarint64ToStream(uint64_t value, std::ostream* stream);

}  // namespace mt

#endif  // MT_VARINT_H_
