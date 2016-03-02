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
#include <boost/filesystem/path.hpp>
#include "multimap/internal/Block.hpp"
#include "multimap/thirdparty/mt/mt.hpp"

namespace multimap {
namespace internal {

class Store : public mt::Resource {
 public:
  struct Options {
    uint32_t block_size = 512;
    uint32_t buffer_size = mt::MiB(1);
    bool readonly = false;
  };

  Store() = default;

  Store(const boost::filesystem::path& file, const Options& options);

  ~Store();

  // ---------------------------------------------------------------------------
  // Public thread-safe interface, no external synchronization needed.
  // ---------------------------------------------------------------------------

  template <bool IsMutable>
  uint32_t put(const Block<IsMutable>& block) {
    MT_REQUIRE_EQ(block.size(), getBlockSize());
    std::lock_guard<std::mutex> lock(mutex_);
    return putUnlocked(block.data());
  }

  template <typename InputIter>
  // InputIter must point to non-const ExtendedBlock<IsMutable>
  void put(InputIter first, InputIter last) {
    if (first != last) {
      std::lock_guard<std::mutex> lock(mutex_);
      while (first != last) {
        if (!first->ignore) {
          MT_REQUIRE_EQ(first->size(), getBlockSize());
          first->id = putUnlocked(first->data());
        }
        ++first;
      }
    }
  }

  void get(uint32_t id, ReadWriteBlock* block) const {
    std::lock_guard<std::mutex> lock(mutex_);
    getUnlocked(id, block->data());
  }

  void get(ExtendedReadWriteBlock* block) const {
    std::lock_guard<std::mutex> lock(mutex_);
    getUnlocked(block->id, block->data());
  }

  template <typename InputIter>
  // InputIter must point to non-const ExtendedReadWriteBlock
  void get(InputIter first, InputIter last) const {
    if (first != last) {
      std::lock_guard<std::mutex> lock(mutex_);
      while (first != last) {
        if (!first->ignore) {
          getUnlocked(first->id, first->data());
        }
        ++first;
      }
    }
  }

  template <bool IsMutable>
  void replace(uint32_t id, const Block<IsMutable>& block) {
    MT_REQUIRE_EQ(block.size(), getBlockSize());
    std::lock_guard<std::mutex> lock(mutex_);
    replaceUnlocked(id, block.data());
  }

  template <bool IsMutable>
  void replace(const ExtendedBlock<IsMutable>& block) {
    MT_REQUIRE_EQ(block.size(), getBlockSize());
    std::lock_guard<std::mutex> lock(mutex_);
    replaceUnlocked(block.id, block.data());
  }

  template <typename InputIter>
  // InputIter must point to ExtendedBlock<IsMutable>
  void replace(InputIter first, InputIter last) {
    if (first != last) {
      std::lock_guard<std::mutex> lock(mutex_);
      while (first != last) {
        if (!first->ignore) {
          MT_REQUIRE_EQ(first->size(), getBlockSize());
          replaceUnlocked(first->id, first->data());
        }
        ++first;
      }
    }
  }

  enum class AccessPattern { NORMAL, WILLNEED };
  // The names are borrowed from `posix_fadvise`.

  void adviseAccessPattern(AccessPattern pattern) const;
  // TODO Replace by `readAhead()` which uses POSIX readahead().

  bool isReadOnly() const { return buffer_.size == 0; }

  uint64_t getBlockSize() const { return options_.block_size; }
  // uint64 is used to promote uint64 conversion
  // of other operands in arithmetic expressions.

  uint64_t getNumBlocks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return getNumBlocksUnlocked();
  }

 private:
  // ---------------------------------------------------------------------------
  // Private non-thread-safe interface, needs external synchronization.
  // ---------------------------------------------------------------------------

  uint32_t putUnlocked(const char* block);

  void getUnlocked(uint32_t id, char* block) const;

  void replaceUnlocked(uint32_t id, const char* block);

  char* getAddressOf(uint32_t id) const;

  uint64_t getNumBlocksUnlocked() const {
    return mapped_.getNumBlocks(options_.block_size) +
           buffer_.getNumBlocks(options_.block_size);
  }

  struct Mapped {
    void* data = nullptr;
    uint64_t size = 0;

    // Requires: `block_size` != 0
    uint64_t getNumBlocks(uint32_t block_size) const {
      return size / block_size;
    }
  };

  struct Buffer {
    std::unique_ptr<char[]> data;
    uint64_t offset = 0;
    uint64_t size = 0;

    void flushTo(int fd) {
      mt::write(fd, data.get(), offset);
      offset = 0;
    }

    bool full() const { return offset == size; }

    bool empty() const { return offset == 0; }

    // Requires: `block_size` != 0
    uint64_t getNumBlocks(uint32_t block_size) const {
      return offset / block_size;
    }
  };

  mutable std::mutex mutex_;
  mutable bool fill_page_cache_ = false;
  mt::AutoCloseFd fd_;
  Options options_;
  Mapped mapped_;
  Buffer buffer_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_STORE_HPP_INCLUDED
