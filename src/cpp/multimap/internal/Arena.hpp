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

#ifndef MULTIMAP_INCLUDE_INTERNAL_ARENA_HPP
#define MULTIMAP_INCLUDE_INTERNAL_ARENA_HPP

#include <memory>
#include <vector>

namespace multimap {
namespace internal {

class Arena {
 public:
  static const std::size_t DEFAULT_BLOCK_SIZE = 4096;

  Arena(std::size_t block_size = DEFAULT_BLOCK_SIZE);

  char* allocate(std::size_t num_bytes);
  // Thread-safe: no.

  void reset();
  // Thread-safe: no.

  std::size_t block_size() const { return block_size_; }
  // Thread-safe: no.

  std::size_t num_blocks() const { return blocks_.size(); }
  // Thread-safe: no.

  std::size_t capacity() const { return capacity_; }
  // Thread-safe: no.

  std::size_t size() const { return size_; }
  // Thread-safe: no.

 private:
  void allocateNewBlock(std::size_t block_size);
  // Thread-safe: no.

  std::vector<std::unique_ptr<char[]>> blocks_;
  std::size_t block_size_;
  std::size_t capacity_;
  std::size_t size_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INCLUDE_INTERNAL_ARENA_HPP
