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

#include "multimap/Bytes.hpp"

#include <cstring>
#include "multimap/Slice.hpp"

namespace multimap {

Bytes makeBytes(const char* cstr) {
  Bytes bytes(std::strlen(cstr));
  std::memcpy(bytes.data(), cstr, bytes.size());
  return bytes;
}

Bytes makeBytes(const std::string& str) { return makeBytes(str.c_str()); }

const byte* parseBytesFromBuffer(const byte* buffer, Bytes* output) {
  const Slice bytes = Slice::readFromBuffer(buffer);
  bytes.copyTo(output);
  return bytes.end();
}

bool parseBytesFromStream(std::FILE* stream, Bytes* output) {
  return Slice::readFromStream(stream, [output](size_t size) {
    output->resize(size);
    return output->data();
  }).size() != 0;
}

byte* serializeBytesToBuffer(byte* begin, byte* end, const Bytes& input) {
  return Slice(input).writeToBuffer(begin, end);
}

void serializeBytesToStream(std::FILE* stream, const Bytes& input) {
  Slice(input).writeToStream(stream);
}

}  // namespace multimap
