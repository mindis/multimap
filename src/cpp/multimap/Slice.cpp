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

#include "multimap/Slice.hpp"

#include "multimap/thirdparty/mt/assert.hpp"
#include "multimap/thirdparty/mt/fileio.hpp"
#include "multimap/thirdparty/mt/varint.hpp"

namespace multimap {

Slice Slice::readFromBuffer(const byte* buffer) {
  uint32_t slice_size = 0;
  buffer += mt::readVarint32FromBuffer(buffer, &slice_size);
  return Slice(buffer, slice_size);
}

size_t Slice::writeToBuffer(byte* begin, byte* end) const {
  byte* pos = begin;
  pos += mt::writeVarint32ToBuffer(size_, pos, end);
  if (static_cast<size_t>(end - pos) >= size_) {
    std::memcpy(pos, data_, size_);
    return std::distance(begin, pos + size_);
  }
  return 0;
}

void Slice::writeToStream(std::FILE* stream) const {
  MT_REQUIRE_LE(size_, std::numeric_limits<uint32_t>::max());
  mt::writeVarint32ToStream(size_, stream);
  mt::fwriteAll(stream, data_, size_);
}

}  // namespace multimap
