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

Bytes toBytes(const char* cstr) {
  Bytes bytes(std::strlen(cstr));
  std::memcpy(bytes.data(), cstr, bytes.size());
  return bytes;
}

Bytes toBytes(const std::string& str) { return toBytes(str.c_str()); }

size_t readBytesFromBuffer(const byte* begin, const byte* end, Bytes* output) {
  const Slice slice = Slice::readFromBuffer(begin, end);
  if (!slice.empty()) {
    slice.copyTo(output);
    return slice.end() - begin;
  }
  return 0;
}

bool readBytesFromStream(std::FILE* stream, Bytes* output) {
  const Slice slice = Slice::readFromStream(stream, [output](size_t size) {
    output->resize(size);
    return output->data();
  });
  return !slice.empty();
}

size_t writeBytesToBuffer(byte* begin, byte* end, const Bytes& input) {
  return Slice(input).writeToBuffer(begin, end);
}

void writeBytesToStream(std::FILE* stream, const Bytes& input) {
  Slice(input).writeToStream(stream);
}

}  // namespace multimap
