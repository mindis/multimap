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

#ifndef MULTIMAP_INTERNAL_ARENA_HPP_INCLUDED
#define MULTIMAP_INTERNAL_ARENA_HPP_INCLUDED

#include <memory>
#include <vector>

namespace multimap {
namespace internal {

class Arena {
  // This class is not thread-safe by design and needs external locking.

 public:
  static const std::size_t DEFAULT_CHUNK_SIZE = 4096;

  explicit Arena(std::size_t chunk_size = DEFAULT_CHUNK_SIZE);

  Arena(Arena&&) = default;
  Arena& operator=(Arena&&) = default;

  char* allocate(std::size_t num_bytes);

  std::size_t allocated() const { return allocated_; }

  void deallocateAll() {
    chunks_.clear();
    blobs_.clear();
    allocated_ = 0;
    offset_ = 0;
  }

 private:
  std::vector<std::unique_ptr<char[]>> chunks_;
  std::vector<std::unique_ptr<char[]>> blobs_;
  std::size_t chunk_size_ = 0;
  std::size_t allocated_ = 0;
  std::size_t offset_ = 0;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_ARENA_HPP_INCLUDED
