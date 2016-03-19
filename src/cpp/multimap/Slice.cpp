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

#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/internal/Varint.hpp"

namespace multimap {

Slice Slice::readFromBuffer(const byte* begin, const byte* end) {
  uint32_t size = 0;
  const size_t nbytes = internal::Varint::readFromBuffer(begin, end, &size);
  return (nbytes != 0) ? Slice(begin + nbytes, size) : Slice();
}

Slice Slice::readFromStream(std::FILE* stream,
                            std::function<byte*(size_t)> allocate) {
  uint32_t size = 0;
  if (internal::Varint::readFromStream(stream, &size) != 0) {
    byte* data = allocate(size);
    mt::read(stream, data, size);
    // The stream is expected to contain valid encodings of Bytes objects.
    // Hence, after successfully reading the size field, mt::read() will
    // throw, if the data field could not be read to signal an invalid stream.
    return Slice(data, size);
  }
  return Slice();
}

size_t Slice::writeToBuffer(byte* begin, byte* end) const {
  MT_REQUIRE_FALSE(empty());
  MT_REQUIRE_LE(begin, end);
  MT_REQUIRE_LE(size_, internal::Varint::Limits::MAX_N4);
  byte* pos = begin + internal::Varint::writeToBuffer(begin, end, size_);
  if ((pos != begin) && (static_cast<size_t>(end - pos) >= size_)) {
    std::memcpy(pos, data_, size_);
    return (pos + size_) - begin;
  }
  return 0;
}

void Slice::writeToStream(std::FILE* stream) const {
  MT_REQUIRE_LE(size_, internal::Varint::Limits::MAX_N4);
  internal::Varint::writeToStream(stream, size_);
  mt::write(stream, data_, size_);
}

}  // namespace multimap
