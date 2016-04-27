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
#include "multimap/thirdparty/mt/fileio.hpp"
#include "multimap/thirdparty/mt/varint.hpp"

namespace multimap {

Bytes toBytes(const char* cstr) {
  Bytes bytes(std::strlen(cstr));
  std::memcpy(bytes.data(), cstr, bytes.size());
  return bytes;
}

Bytes toBytes(const std::string& str) { return toBytes(str.c_str()); }

size_t readBytesFromBuffer(const byte* buffer, Bytes* output) {
  const Slice slice = Slice::readFromBuffer(buffer);
  slice.copyTo(output);
  return slice.end() - buffer;
}

size_t writeBytesToBuffer(const Bytes& input, byte* begin, byte* end) {
  return Slice(input).writeToBuffer(begin, end);
}

bool readBytesFromStream(std::FILE* stream, Bytes* output) {
  uint32_t num_bytes = 0;
  if (mt::readVarint32FromStream(stream, &num_bytes)) {
    output->resize(num_bytes);
    mt::freadAll(stream, output->data(), output->size());
    // The stream is expected to contain valid encodings of Bytes objects.
    // Hence, after successfully reading the size field, mt::freadAll() will
    // throw, if the data field could not be read to signal an invalid stream.
    return true;
  }
  return false;
}

void writeBytesToStream(const Bytes& input, std::FILE* stream) {
  Slice(input).writeToStream(stream);
}

}  // namespace multimap
