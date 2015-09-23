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

#include <algorithm>
#include "multimap/internal/thirdparty/mt.hpp"

namespace multimap {
namespace internal {

Arena::Arena(std::size_t block_size)
    : block_size_(block_size), capacity_(0), size_(0) {
  mt::check(block_size > 0, "Arena::Arena: block_size must be positive");
}

char* Arena::allocate(std::size_t num_bytes) {
  mt::check(num_bytes > 0, "Arena::Allocate: num_bytes must be positive");
  const auto num_bytes_free = capacity_ - size_;
  if (num_bytes_free < num_bytes) {
    allocateNewBlock(std::max(block_size_, num_bytes));
  }
  const auto data = blocks_.back().get() + size_;
  size_ += num_bytes;
  return data;
}

void Arena::reset() {
  blocks_.clear();
  capacity_ = 0;
  size_ = 0;
}

void Arena::allocateNewBlock(std::size_t block_size) {
  blocks_.emplace_back(new char[block_size]);
  capacity_ = block_size;
  size_ = 0;
}

}  // namespace internal
}  // namespace multimap
