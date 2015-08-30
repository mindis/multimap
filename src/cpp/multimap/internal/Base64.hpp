// This file is part of the Multimap library.  http://multimap.io
//
// Copyright (C) 2015  Martin Trenkmann
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

#ifndef MULTIMAP_INTERNAL_BASE64_HPP
#define MULTIMAP_INTERNAL_BASE64_HPP

#include <string>
#include "multimap/Bytes.hpp"

namespace multimap {
namespace internal {

struct Base64 {
  // Encodes binary data to a base64 string.
  static void Encode(const Bytes& binary, std::string* base64);

  // Decodes a base64 string to binary data. std::string as the target type
  // is used as a self-managing byte buffer, it will contain binary data.
  static void Decode(const std::string& base64, std::string* binary);

  Base64() = delete;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_BASE64_HPP
