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

#ifndef MULTIMAP_INTERNAL_STORE_HPP_INCLUDED
#define MULTIMAP_INTERNAL_STORE_HPP_INCLUDED

#include <memory>
#include <mutex>
#include <unordered_map>
#include <boost/filesystem/path.hpp>
#include "multimap/internal/UintVector.hpp"
#include "multimap/Slice.hpp"

namespace multimap {
namespace internal {

class List;

class Store {
 public:
  struct Options {
    uint32_t block_size = 128;  // 512
    uint32_t mmap_segment_size = mt::MiB(10);
    bool readonly = false;
  };

  struct Block {
    byte* data = nullptr;
    uint32_t offset = 0;
    uint32_t size = 0;

    byte* begin() const { return data; }

    byte* current() const { return data + offset; }

    byte* end() const { return data + size; }
  };

  Store() = default;

  Store(const boost::filesystem::path& file, const Options& options);

  uint32_t put(const Block& block) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (mappings_.empty() || mappings_.back().isFull()) {
      const uint64_t old_filesize = mt::seek(fd_.get(), 0, SEEK_END);
      const uint64_t new_filesize = old_filesize + options_.mmap_segment_size;
      mt::truncate(fd_.get(), new_filesize);
      mappings_.push_back(mt::mmap(options_.mmap_segment_size,
                                   PROT_READ | PROT_WRITE, MAP_SHARED,
                                   fd_.get(), old_filesize));
    }
    mappings_.back().append(block);
    return numBlocksUsed() - 1;
  }

  uint32_t getBlockSize() const { return options_.block_size; }

 private:
  uint32_t numBlocksAllocated() const {
    const uint64_t allocated = mappings_.size() * options_.mmap_segment_size;
    return allocated / options_.block_size;
  }

  uint32_t numBlocksUsed() const {
    return !mappings_.empty()
               ? (numBlocksAllocated() -
                  mappings_.back().numBlocksFree(options_.block_size))
               : 0;
  }

  class Mapping {
   public:
    Mapping(mt::AutoUnmapMemory memory) : memory_(std::move(memory)) {}

    void append(const Block& block) {
      MT_REQUIRE_FALSE(isFull());
      std::memcpy(memory_.addr() + offset_, block.data, block.size);
      offset_ += block.size;
    }

    bool isFull() const { return offset_ == memory_.length(); }

    int numBlocksAllocated(int block_size) const {
      return memory_.length() / block_size;
    }

    int numBlocksUsed(int block_size) const { return offset_ / block_size; }

    int numBlocksFree(int block_size) const {
      return (memory_.length() - offset_) / block_size;
    }

   private:
    mt::AutoUnmapMemory memory_;
    uint32_t offset_ = 0;
  };

  mutable std::mutex mutex_;
  std::vector<Mapping> mappings_;
  mt::AutoCloseFd fd_;
  Options options_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_STORE_HPP_INCLUDED
