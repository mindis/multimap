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

#include "multimap/internal/TsvFileWriter.hpp"

#include "multimap/internal/Base64.hpp"
#include "multimap/thirdparty/mt/assert.hpp"
#include "multimap/thirdparty/mt/check.hpp"

namespace multimap {
namespace internal {

TsvFileWriter::TsvFileWriter(const boost::filesystem::path& file_path)
    : stream_(file_path) {
  mt::check(stream_.is_open(), "Could not create %s", file_path.c_str());
}

void TsvFileWriter::write(const Slice& key, const Slice& value) {
  MT_REQUIRE_FALSE(key.empty());
  MT_REQUIRE_FALSE(value.empty());
  if (key != current_key_) {
    if (!current_key_.empty()) {
      stream_ << '\n';
    }
    Base64::encode(key, &base64_key_);
    stream_ << base64_key_;
    key.copyTo(&current_key_);
  }
  Base64::encode(value, &base64_value_);
  stream_ << '\t' << base64_value_;
}

void TsvFileWriter::write(const Slice& key, Iterator* iter) {
  MT_REQUIRE_FALSE(key.empty());
  MT_REQUIRE_NOT_ZERO(iter->available());
  if (key != current_key_) {
    if (!current_key_.empty()) {
      stream_ << '\n';
    }
    Base64::encode(key, &base64_key_);
    stream_ << base64_key_;
    key.copyTo(&current_key_);
  }
  while (iter->hasNext()) {
    Base64::encode(iter->next(), &base64_value_);
    stream_ << '\t' << base64_value_;
  }
}

}  // namespace internal
}  // namespace multimap
