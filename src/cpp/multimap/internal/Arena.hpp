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

#ifndef MULTIMAP_INTERNAL_ARENA_HPP_INCLUDED
#define MULTIMAP_INTERNAL_ARENA_HPP_INCLUDED

#include <memory>
#include <mutex>
#include <vector>
#include "multimap/thirdparty/mt/mt.hpp"

namespace multimap {
namespace internal {

class Arena : private mt::Resource {
  // Objects of this class are thread-safe.

public:
  static const uint32_t DEFAULT_CHUNK_SIZE = 4096;

  explicit Arena(uint32_t chunk_size = DEFAULT_CHUNK_SIZE);

  char* allocate(uint32_t nbytes);

  uint64_t allocated() const;

  void deallocateAll();

private:
  mutable std::mutex mutex_;
  std::vector<std::unique_ptr<char[]> > chunks_;
  std::vector<std::unique_ptr<char[]> > blobs_;
  uint32_t chunk_offset_ = 0;
  uint32_t chunk_size_ = 0;
  uint64_t allocated_ = 0;
};

} // namespace internal
} // namespace multimap

#endif // MULTIMAP_INTERNAL_ARENA_HPP_INCLUDED
