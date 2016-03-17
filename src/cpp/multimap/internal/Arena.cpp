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

Arena::Arena(size_t block_size)
    : mutex_(new std::mutex()), block_size_(block_size) {
  MT_REQUIRE_TRUE(mt::isPowerOfTwo(block_size_));
  MT_REQUIRE_NOT_ZERO(block_size_);
}

byte* Arena::allocate(size_t nbytes) {
  MT_REQUIRE_NOT_ZERO(nbytes);
  std::lock_guard<std::mutex> lock(*mutex_);

  byte* result;
  if (nbytes <= block_size_) {
    if (blocks_.empty()) {
      blocks_.emplace_back(new byte[block_size_]);
      block_offset_ = 0;
    }
    const auto num_bytes_free = block_size_ - block_offset_;
    if (nbytes > num_bytes_free) {
      blocks_.emplace_back(new byte[block_size_]);
      block_offset_ = 0;
    }
    result = blocks_.back().get() + block_offset_;
    block_offset_ += nbytes;

  } else {
    blobs_.emplace_back(new byte[nbytes]);
    result = blobs_.back().get();
  }

  allocated_ += nbytes;
  return result;
}

size_t Arena::allocated() const {
  std::lock_guard<std::mutex> lock(*mutex_);
  return allocated_;
}

void Arena::deallocateAll() {
  std::lock_guard<std::mutex> lock(*mutex_);
  blocks_.clear();
  blobs_.clear();
  block_offset_ = 0;
  allocated_ = 0;
}

}  // namespace internal
}  // namespace multimap
