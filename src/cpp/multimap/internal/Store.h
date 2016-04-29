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

#ifndef MULTIMAP_INTERNAL_STORE_H_
#define MULTIMAP_INTERNAL_STORE_H_

#include <mutex>
#include <vector>
#include <boost/filesystem/path.hpp>
#include "multimap/thirdparty/mt/fileio.h"
#include "multimap/thirdparty/mt/memory.h"
#include "multimap/Bytes.h"
#include "multimap/Options.h"

namespace multimap {
namespace internal {

class List;

class Store {
 public:
  struct Block {
    byte* data = nullptr;
    uint32_t offset = 0;
    uint32_t size = 0;

    byte* begin() const { return data; }

    byte* end() const { return data + size; }

    byte* cur() const { return data + offset; }

    size_t remaining() const { return size - offset; }

    bool empty() const { return size == 0; }

    void clear() {
      data = nullptr;
      offset = 0;
      size = 0;
    }

    Block() = default;
  };

  typedef std::vector<uint32_t> BlockIds;
  typedef std::vector<Block> Blocks;

  Store();

  Store(Store&&) = default;
  Store& operator=(Store&&) = default;

  Store(const boost::filesystem::path& file, const Options& options);

  ~Store();

  uint32_t put(const Block& block);

  Blocks get(const BlockIds& block_ids) const;

  size_t getNumBlocks() const {
    std::lock_guard<std::mutex> lock(*mutex_);
    return getNumBlocksUnlocked();
  }

  size_t getBlockSize() const { return options_.block_size; }

  bool isReadOnly() const { return options_.readonly; }

 private:
  size_t getNumBlocksUnlocked() const;

  class Segment {
   public:
    Segment(mt::AutoUnmapMemory memory);

    Segment(mt::AutoUnmapMemory memory, size_t offset);

    void append(const Block& block);

    Block get(uint32_t block_id, size_t block_size) const;

    bool isFull() const { return offset_ == memory_.size(); }

    size_t getNumBlocks(size_t block_size) const {
      return offset_ / block_size;
    }

   private:
    mt::AutoUnmapMemory memory_;
    size_t offset_ = 0;
  };

  std::unique_ptr<std::mutex> mutex_;
  std::vector<Segment> segments_;
  mt::AutoCloseFd fd_;
  Options options_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_STORE_H_
