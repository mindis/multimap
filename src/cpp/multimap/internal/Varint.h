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

#ifndef MULTIMAP_INTERNAL_VARINT_H_
#define MULTIMAP_INTERNAL_VARINT_H_

#include <cstdint>
#include "multimap/Bytes.h"

namespace multimap {
namespace internal {

struct Varint {
  static const uint32_t MAX_VARINT_WITH_FLAG;

  static size_t readFromBuffer(const byte* begin, const byte* end,
                               uint32_t* value, bool* flag);

  static size_t writeToBuffer(byte* begin, byte* end, uint32_t value,
                              bool flag);

  static void setFlagInBuffer(byte* buffer, bool flag);

  Varint() = delete;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_VARINT_H_
