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

namespace multimap {
namespace internal {

Arena::Arena(std::size_t chunk_size) : chunk_size_(chunk_size) {
  MT_REQUIRE_TRUE(mt::isPowerOfTwo(chunk_size_));
  MT_REQUIRE_NOT_ZERO(chunk_size_);
}

char* Arena::allocate(std::size_t num_bytes) {
  MT_REQUIRE_NOT_ZERO(num_bytes);
  std::lock_guard<std::mutex> lock(mutex_);

  char* result;
  if (num_bytes <= chunk_size_) {
    if (chunks_.empty()) {
      chunks_.emplace_back(new char[chunk_size_]);
      offset_ = 0;
    }
    const auto num_bytes_free = chunk_size_ - offset_;
    if (num_bytes > num_bytes_free) {
      chunks_.emplace_back(new char[chunk_size_]);
      offset_ = 0;
    }
    result = chunks_.back().get() + offset_;
    offset_ += num_bytes;

  } else {
    blobs_.emplace_back(new char[num_bytes]);
    result = blobs_.back().get();
  }

  allocated_ += num_bytes;
  return result;
}

std::size_t Arena::allocated() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return allocated_;
}

void Arena::deallocateAll() {
  std::lock_guard<std::mutex> lock(mutex_);
  chunks_.clear();
  blobs_.clear();
  allocated_ = 0;
  offset_ = 0;
}

} // namespace internal
} // namespace multimap
