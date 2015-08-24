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

#ifndef MULTIMAP_INTERNAL_BLOCK_POOL_HPP
#define MULTIMAP_INTERNAL_BLOCK_POOL_HPP

#include <memory>
#include <mutex>
#include <vector>
#include "multimap/internal/Block.hpp"

namespace multimap {
namespace internal {

class BlockPool {
 public:
  BlockPool();

  BlockPool(std::size_t num_blocks, std::size_t block_size);

  void Init(std::size_t num_blocks, std::size_t block_size);

  // Thread-safe: yes.
  Block Pop();

  // Thread-safe: yes.
  void Push(Block&& block);

  // Thread-safe: yes.
  void Push(std::vector<Block>* blocks);

  // Thread-safe: yes.
  std::size_t num_blocks() const;

  // Thread-safe: yes.
  std::size_t block_size() const;

  // Thread-safe: yes.
  std::uint64_t memory() const;

  // Thread-safe: yes.
  std::size_t num_blocks_free() const;

  // Thread-safe: yes.
  bool empty() const;

  // Thread-safe: yes.
  bool full() const;

 private:
  // Thread-safe: yes.
  bool valid(byte* ptr) const;

  // THread-safe: no.
  void PushUnlocked(Block&& block);

  std::size_t num_blocks_;
  std::size_t block_size_;
  std::uint64_t end_of_data_;
  std::unique_ptr<byte[]> data_;
  std::vector<std::uint32_t> ids_;
  mutable std::mutex mutex_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_BLOCK_POOL_HPP
