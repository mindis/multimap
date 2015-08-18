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

BlockPool::BlockPool() : num_blocks_(0), block_size_(0), end_of_data_(0) {}

BlockPool::BlockPool(std::size_t num_blocks, std::size_t block_size) {
  Init(num_blocks, block_size);
}

void BlockPool::Init(std::size_t num_blocks, std::size_t block_size) {
  num_blocks_ = num_blocks;
  block_size_ = block_size;
  end_of_data_ = num_blocks * block_size;
  data_.reset(new byte[end_of_data_]);
  ids_.resize(num_blocks);
  for (std::size_t i = 0; i != ids_.size(); ++i) {
    ids_[i] = i;
  }
}

Block BlockPool::Pop() {
  Block block;
  const std::lock_guard<std::mutex> lock(mutex_);
  if (!ids_.empty()) {
    auto block_data = data_.get() + ids_.back() * block_size_;
    block.set_data(block_data, block_size_);
    ids_.pop_back();
  }
  return block;
}

void BlockPool::Push(Block&& block) {
  const std::lock_guard<std::mutex> lock(mutex_);
  PushUnlocked(std::move(block));
}

void BlockPool::Push(std::vector<Block>* blocks) {
  const std::lock_guard<std::mutex> lock(mutex_);
  for (auto& block : *blocks) {
    PushUnlocked(std::move(block));
  }
}

std::size_t BlockPool::num_blocks() const { return num_blocks_; }

std::size_t BlockPool::block_size() const { return block_size_; }

std::uint64_t BlockPool::memory() const { return end_of_data_; }

std::size_t BlockPool::num_blocks_free() const {
  const std::lock_guard<std::mutex> lock(mutex_);
  return ids_.size();
}

bool BlockPool::empty() const {
  const std::lock_guard<std::mutex> lock(mutex_);
  return ids_.empty();
}

bool BlockPool::full() const {
  const std::lock_guard<std::mutex> lock(mutex_);
  return ids_.size() == num_blocks_;
}

bool BlockPool::valid(byte* ptr) const {
  const auto end_of_range = data_.get() + end_of_data_;
  const auto ptr_in_range = ptr >= data_.get() && ptr < end_of_range;
  const auto block_aligned = (ptr - data_.get()) % block_size_ == 0;
  return ptr_in_range && block_aligned;
}

void BlockPool::PushUnlocked(Block&& block) {
  assert(block.has_data());
  assert(valid(block.data()));
  assert(ids_.size() < num_blocks_);
  ids_.push_back((block.data() - data_.get()) / block_size_);
}

}  // namespace internal
}  // namespace multimap
