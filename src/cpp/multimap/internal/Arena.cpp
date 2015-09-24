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

#include "multimap/internal/Arena.hpp"

#include "multimap/internal/thirdparty/mt.hpp"

namespace multimap {
namespace internal {

Arena::Arena(std::size_t chunk_size) {
  MT_REQUIRE_NOT_ZERO(chunk_size);
  MT_REQUIRE_TRUE(mt::isPowerOfTwo(chunk_size));
  chunk_size_ = chunk_size;
  offset_ = chunk_size;  // First request will trigger allocation.
}

char* Arena::allocate(std::size_t num_bytes) {
  MT_REQUIRE_NOT_ZERO(num_bytes);

  const auto num_bytes_free = chunk_size_ - offset_;
  if (num_bytes > num_bytes_free) {
    if (num_bytes > chunk_size_) {
      blobs_.emplace_back(new char[num_bytes]);
      allocated_ += num_bytes;
      return blobs_.back().get();
    }
    chunks_.emplace_back(new char[chunk_size_]);
    offset_ = 0;
  }
  const auto result = chunks_.back().get() + offset_;
  allocated_ += num_bytes;
  offset_ += num_bytes;
  return result;
}

}  // namespace internal
}  // namespace multimap
