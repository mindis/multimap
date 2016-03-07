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

#include "multimap/internal/Arena.hpp"

#include "multimap/thirdparty/mt/mt.hpp"

namespace multimap {
namespace internal {

Arena::Arena(uint32_t chunk_size)
    : mutex_(new std::mutex()), chunk_size_(chunk_size) {
  MT_REQUIRE_TRUE(mt::isPowerOfTwo(chunk_size_));
  MT_REQUIRE_NOT_ZERO(chunk_size_);
}

uint8_t* Arena::allocate(uint32_t num_bytes) {
  MT_REQUIRE_NOT_ZERO(num_bytes);
  std::lock_guard<std::mutex> lock(*mutex_);

  uint8_t* result;
  if (num_bytes <= chunk_size_) {
    if (chunks_.empty()) {
      chunks_.emplace_back(new uint8_t[chunk_size_]);
      chunk_offset_ = 0;
    }
    const auto num_bytes_free = chunk_size_ - chunk_offset_;
    if (num_bytes > num_bytes_free) {
      chunks_.emplace_back(new uint8_t[chunk_size_]);
      chunk_offset_ = 0;
    }
    result = chunks_.back().get() + chunk_offset_;
    chunk_offset_ += num_bytes;

  } else {
    blobs_.emplace_back(new uint8_t[num_bytes]);
    result = blobs_.back().get();
  }

  allocated_ += num_bytes;
  return result;
}

uint64_t Arena::allocated() const {
  std::lock_guard<std::mutex> lock(*mutex_);
  return allocated_;
}

void Arena::deallocateAll() {
  std::lock_guard<std::mutex> lock(*mutex_);
  chunks_.clear();
  blobs_.clear();
  chunk_offset_ = 0;
  allocated_ = 0;
}

}  // namespace internal
}  // namespace multimap
