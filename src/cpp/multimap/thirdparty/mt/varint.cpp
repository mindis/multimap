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

#include "mt/varint.h"

#include "mt/fileio.h"

namespace mt {

// The varint implementations in [1] and [2] might be slightly more efficient,
// but my own long running benchmarks haven't shown any significant difference
// to the straight forward solution given here.
// [1] https://github.com/google/protobuf/blob/master/src/google/protobuf/io/coded_stream.cc
// [2] https://github.com/facebook/folly/blob/master/folly/Varint.h

size_t readVarint32FromBuffer(const byte* buffer, uint32_t* value) {
  return readVarint32FromBuffer(buffer, buffer + MAX_VARINT32_BYTES, value);
}

size_t readVarint32FromBuffer(const byte* begin, const byte* end,
                              uint32_t* value) {
  *value = 0;
  int shift = 0;
  if (MT_LIKELY((end - begin) >= MAX_VARINT32_BYTES)) {
    for (int i = 0; i < MAX_VARINT32_BYTES; i++) {
      *value += static_cast<uint32_t>(*begin & 0x7f) << shift;
      if (!(*begin++ & 0x80)) return i + 1;
      shift += 7;
    }
  } else {
    for (int i = 0; (begin < end) && (i < MAX_VARINT32_BYTES); i++) {
      *value += static_cast<uint32_t>(*begin & 0x7f) << shift;
      if (!(*begin++ & 0x80)) return i + 1;
      shift += 7;
    }
  }
  return 0;
}

size_t readVarint64FromBuffer(const byte* buffer, uint64_t* value) {
  return readVarint64FromBuffer(buffer, buffer + MAX_VARINT64_BYTES, value);
}

size_t readVarint64FromBuffer(const byte* begin, const byte* end,
                              uint64_t* value) {
  *value = 0;
  int shift = 0;
  if (MT_LIKELY((end - begin) >= MAX_VARINT64_BYTES)) {
    for (int i = 0; i < MAX_VARINT64_BYTES; i++) {
      *value += static_cast<uint64_t>(*begin & 0x7f) << shift;
      if (!(*begin++ & 0x80)) return i + 1;
      shift += 7;
    }
  } else {
    for (int i = 0; (begin < end) && (i < MAX_VARINT64_BYTES); i++) {
      *value += static_cast<uint64_t>(*begin & 0x7f) << shift;
      if (!(*begin++ & 0x80)) return i + 1;
      shift += 7;
    }
  }
  return 0;
}

size_t writeVarint32ToBuffer(uint32_t value, byte* begin, byte* end) {
  return writeVarint64ToBuffer(value, begin, end);
}

size_t writeVarint64ToBuffer(uint64_t value, byte* begin, byte* end) {
  byte* pos = begin;
  while ((pos != end) && (value > 0x7f)) {
    *pos++ = 0x80 | (value & 0x7f);
    value >>= 7;
  }
  if (pos == end) return 0;  // Insufficient space in buffer.
  *pos++ = value;
  return pos - begin;
}

bool readVarint32FromStream(std::FILE* stream, uint32_t* value) {
  byte b;
  *value = 0;
  int shift = 0;
  for (int i = 0; (i != MAX_VARINT32_BYTES) && fgetcMaybe(stream, &b); i++) {
    *value += static_cast<uint32_t>(b & 0x7f) << shift;
    if (!(b & 0x80)) return true;
    shift += 7;
  }
  return false;
}

bool readVarint64FromStream(std::FILE* stream, uint64_t* value) {
  byte b;
  *value = 0;
  int shift = 0;
  for (int i = 0; i != MAX_VARINT64_BYTES; i++) {
    if (!fgetcMaybe(stream, &b)) return false;
    *value += static_cast<uint64_t>(b & 0x7f) << shift;
    if (!(b & 0x80)) return true;
    shift += 7;
  }
  return false;
}

void writeVarint32ToStream(uint32_t value, std::FILE* stream) {
  writeVarint64ToStream(value, stream);
}

void writeVarint64ToStream(uint64_t value, std::FILE* stream) {
  while (value > 0x7f) {
    fputc(stream, 0x80 | (value & 0x7f));
    value >>= 7;
  }
  fputc(stream, value);
}

}  // namespace mt
