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

#ifndef MULTIMAP_INCLUDE_INTERNAL_BLOCK_POOL_HPP
#define MULTIMAP_INCLUDE_INTERNAL_BLOCK_POOL_HPP

#include <memory>
#include <mutex>
#include <vector>
#include "multimap/common.hpp"
#include "multimap/internal/Block.hpp"

namespace multimap {
namespace internal {

class BlockPool {
 public:
  BlockPool() = default;

  BlockPool(std::size_t block_size, std::size_t chunk_size = MiB(100));

  void init(std::size_t block_size, std::size_t chunk_size = MiB(100));

  // Thread-safe: yes.
  Block allocate();

  // Thread-safe: yes.
  std::size_t block_size() const;

  // Thread-safe: yes.
  std::size_t chunk_size() const;

  // Thread-safe: yes.
  std::size_t num_blocks() const;

  // Thread-safe: yes.
  std::size_t num_chunks() const;

 private:
  mutable std::mutex mutex_;
  std::size_t block_size_ = 0;
  std::size_t chunk_size_ = 0;
  std::size_t get_offset_ = 0;
  std::vector<std::unique_ptr<char[]>> chunks_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INCLUDE_INTERNAL_BLOCK_POOL_HPP
