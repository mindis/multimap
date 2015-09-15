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

#include "multimap/internal/BlockPool.hpp"

#include <cassert>

namespace multimap {
namespace internal {

// namespace {

// typedef std::vector<std::unique_ptr<char[]>> Chunks;

// char* BlockIdToPtr(std::size_t block_id, std::size_t block_size,
//                   std::size_t chunk_size, const Chunks& chunks) {
//  const auto absolute_offset = block_id * block_size;
//  const auto relative_offset_in_chunk = absolute_offset % chunk_size;
//  const auto chunk_id = absolute_offset / chunk_size;
//  assert(chunk_id < chunks.size());
//  return chunks[chunk_id].get() + relative_offset_in_chunk;
//}

// std::size_t PtrToBlockId(const char* ptr, std::size_t block_size,
//                         std::size_t chunk_size, const Chunks& chunks) {
//  assert(ptr);
//  const auto blocks_per_chunk = chunk_size / block_size;
//  for (std::size_t i = 0; i != chunks.size(); ++i) {
//    const auto beg_of_chunk = chunks[i].get();
//    const auto end_of_chunk = beg_of_chunk + chunk_size;
//    if (ptr >= beg_of_chunk && ptr < end_of_chunk) {
//      assert((ptr - beg_of_chunk) % block_size == 0);
//      const auto relative_id_in_chunk = (ptr - beg_of_chunk) / block_size;
//      const auto absolute_id = relative_id_in_chunk + (blocks_per_chunk * i);
//      return absolute_id;
//    }
//  }
//  return -1;
//}

//}  // namespace

BlockPool::BlockPool(std::size_t block_size, std::size_t chunk_size) {
  init(block_size, chunk_size);
}

void BlockPool::init(std::size_t block_size, std::size_t chunk_size) {
  assert(chunk_size % block_size == 0);
  assert(chunk_size > block_size);
  assert(chunk_size != 0);
  assert(block_size != 0);

  const std::lock_guard<std::mutex> lock(mutex_);

  block_size_ = block_size;
  chunk_size_ = chunk_size;
  get_offset_ = chunk_size;  // Tiggers chunk allocation in Allocate().
  chunks_.clear();
}

Block BlockPool::allocate() {
  const std::lock_guard<std::mutex> lock(mutex_);
  if (get_offset_ == chunk_size_) {
    chunks_.emplace_back(new char[chunk_size_]);
    get_offset_ = 0;
  }
  const auto block_data = chunks_.back().get() + get_offset_;
  get_offset_ += block_size_;
  return Block(block_data, block_size_);
}

std::size_t BlockPool::block_size() const { return block_size_; }

std::size_t BlockPool::chunk_size() const { return chunk_size_; }

std::size_t BlockPool::num_blocks() const {
  const std::lock_guard<std::mutex> lock(mutex_);
  const auto full_chunks = chunks_.empty() ? 0 : (chunks_.size() - 1);
  const auto blocks_per_chunk = chunk_size_ / block_size_;
  const auto blocks_in_last_chunk = get_offset_ / block_size_;
  return full_chunks * blocks_per_chunk + blocks_in_last_chunk;
}

std::size_t BlockPool::num_chunks() const {
  const std::lock_guard<std::mutex> lock(mutex_);
  return chunks_.size();
}

}  // namespace internal
}  // namespace multimap
