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

#include "multimap/internal/BlockArena.hpp"

namespace multimap {
namespace internal {

BlockArena::BlockArena(std::size_t block_size, std::size_t chunk_size) {
  init(block_size, chunk_size);
}

void BlockArena::init(std::size_t block_size, std::size_t chunk_size) {
  MT_REQUIRE_EQ(chunk_size % block_size, 0);
  MT_REQUIRE_GT(chunk_size, block_size);
  MT_REQUIRE_NE(chunk_size, 0);
  MT_REQUIRE_NE(block_size, 0);

  const std::lock_guard<std::mutex> lock(mutex_);

  block_size_ = block_size;
  chunk_size_ = chunk_size;
  get_offset_ = chunk_size;  // Tiggers chunk allocation in Allocate().
  chunks_.clear();
}

Block BlockArena::allocate() {
  const std::lock_guard<std::mutex> lock(mutex_);
  if (get_offset_ == chunk_size_) {
    chunks_.emplace_back(new char[chunk_size_]);
    get_offset_ = 0;
  }
  const auto block_data = chunks_.back().get() + get_offset_;
  get_offset_ += block_size_;
  return Block(block_data, block_size_);
}

std::size_t BlockArena::block_size() const { return block_size_; }

std::size_t BlockArena::chunk_size() const { return chunk_size_; }

std::size_t BlockArena::num_blocks() const {
  const std::lock_guard<std::mutex> lock(mutex_);
  const auto full_chunks = chunks_.empty() ? 0 : (chunks_.size() - 1);
  const auto blocks_per_chunk = chunk_size_ / block_size_;
  const auto blocks_in_last_chunk = get_offset_ / block_size_;
  return full_chunks * blocks_per_chunk + blocks_in_last_chunk;
}

std::size_t BlockArena::num_chunks() const {
  const std::lock_guard<std::mutex> lock(mutex_);
  return chunks_.size();
}

}  // namespace internal
}  // namespace multimap
