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

#include "multimap/internal/TsvFileReader.hpp"

#include "multimap/internal/Base64.hpp"
#include "multimap/thirdparty/mt/mt.hpp"

namespace multimap {
namespace internal {

TsvFileReader::TsvFileReader(const boost::filesystem::path& filename)
    : stream_(filename.string()) {
  mt::check(stream_.is_open(), "Could not open '%s'", filename.c_str());
  stream_ >> base64_key_;
  Base64::decode(base64_key_, &current_key_);
}

bool TsvFileReader::read(Bytes* key, Bytes* value) {
  while (stream_) {
    switch (stream_.peek()) {
      case '\n':
      case '\r':
        stream_ >> base64_key_;
        Base64::decode(base64_key_, &current_key_);
        break;
      case '\f':
      case '\t':
      case '\v':
      case ' ':
        stream_.ignore();
        break;
      default:
        stream_ >> base64_value_;
        Base64::decode(base64_value_, value);
        *key = current_key_;
        return true;
    }
  }
  return false;
}

}  // namespace internal
}  // namespace multimap
