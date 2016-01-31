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

#ifndef MULTIMAP_INTERNAL_BASE64_HPP_INCLUDED
#define MULTIMAP_INTERNAL_BASE64_HPP_INCLUDED

#include "multimap/Bytes.hpp"

namespace multimap {
namespace internal {

struct Base64 {
  static std::string encode(const Bytes& binary);
  // Encodes binary data to a Base64 string.

  static void encode(const Bytes& binary, std::string* base64);
  // Encodes binary data to a Base64 string.

  static void encode(const std::string& binary, std::string* base64);
  // Encodes binary data to a Base64 string.

  static void encode(const char* data, size_t size, std::string* base64);
  // Encodes binary data to a Base64 string.

  static void decode(const std::string& base64, std::string* binary);
  // Decodes a base64 string to binary data. std::string as the target type
  // is used as a self-managing byte buffer, it will contain binary data.

  Base64() = delete;
};

} // namespace internal
} // namespace multimap

#endif // MULTIMAP_INTERNAL_BASE64_HPP_INCLUDED
