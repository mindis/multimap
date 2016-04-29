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

// -----------------------------------------------------------------------------
// Documentation:  https://multimap.io/cppreference/#arenahpp
// -----------------------------------------------------------------------------

#ifndef MULTIMAP_ARENA_H_
#define MULTIMAP_ARENA_H_

#include <memory>
#include <mutex>
#include <vector>
#include "multimap/thirdparty/mt/common.h"
#include "multimap/Bytes.h"

namespace multimap {

class Arena {
 public:
  static const size_t DEFAULT_BLOCK_SIZE = mt::KiB(4);

  explicit Arena(size_t block_size = DEFAULT_BLOCK_SIZE);

  byte* allocate(size_t nbytes);

  size_t allocated() const;

  void deallocateAll();

 private:
  std::unique_ptr<std::mutex> mutex_;
  std::vector<std::unique_ptr<byte[]> > blocks_;
  std::vector<std::unique_ptr<byte[]> > blobs_;
  size_t block_offset_ = 0;
  size_t block_size_ = 0;
  size_t allocated_ = 0;
};

}  // namespace multimap

#endif  // MULTIMAP_ARENA_H_
