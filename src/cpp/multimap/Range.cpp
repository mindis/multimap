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

#include "multimap/Range.hpp"

#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/internal/Varint.hpp"

namespace multimap {

Range Range::readFromBuffer(const byte* buffer) {
  uint32_t size = 0;
  buffer = internal::Varint::readFromBuffer(buffer, &size);
  return Range(buffer, size);
}

Range Range::readFromStream(std::FILE* stream,
                            std::function<byte*(size_t)> allocate) {
  uint32_t size = 0;
  if (internal::Varint::readFromStream(stream, &size)) {
    byte* data = allocate(size);
    mt::read(stream, data, size);
    // The stream is expected to contain valid encodings of Bytes objects.
    // Hence, after successfully reading the size field, mt::fread() will
    // throw, if the data field could not be read to signal an invalid stream.
    return Range(data, size);
  }
  return Range();
}

byte* Range::writeToBuffer(byte* begin, byte* end) const {
  MT_REQUIRE_LE(begin, end);
  const size_t count = size();
  MT_ASSERT_LE(count, internal::Varint::Limits::MAX_N4);
  byte* new_begin = internal::Varint::writeToBuffer(begin, end, count);
  if ((new_begin != begin) && (static_cast<size_t>(end - new_begin) >= count)) {
    std::memcpy(new_begin, begin_, count);
    new_begin += count;
    return new_begin;
  }
  return begin;
}

void Range::writeToStream(std::FILE* stream) const {
  const size_t count = size();
  MT_ASSERT_LE(count, internal::Varint::Limits::MAX_N4);
  internal::Varint::writeToStream(stream, count);
  mt::write(stream, begin_, count);
}

}  // namespace multimap
