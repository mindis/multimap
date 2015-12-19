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

#ifndef MULTIMAP_INTERNAL_STORE_HPP_INCLUDED
#define MULTIMAP_INTERNAL_STORE_HPP_INCLUDED

#include <memory>
#include <mutex>
#include <type_traits>
#include <boost/filesystem/path.hpp>
#include "multimap/internal/Block.hpp"
#include "multimap/thirdparty/mt/mt.hpp"

namespace multimap {
namespace internal {

class Store : mt::Resource {
 public:
  struct Options {
    std::size_t block_size = 512;
    std::size_t buffer_size = mt::MiB(1);
    bool create_if_missing = false;
    bool readonly = false;
    bool quiet = false;
  };

  struct Stats {
    std::uint64_t block_size = 0;
    std::uint64_t num_blocks = 0;

    static Stats fromProperties(const mt::Properties& properties);

    mt::Properties toProperties() const;

    static Stats readFromFd(int fd);

    void writeToFd(int fd) const;
  };

  static_assert(std::is_standard_layout<Stats>::value,
                "Store::Stats is no standard layout type");

  static_assert(mt::hasExpectedSize<Stats>(16, 16),
                "Store::Stats does not have expected size");
  // sizeof(Stats) must be equal on 32- and 64-bit systems to be portable.

  explicit Store(const boost::filesystem::path& file);

  Store(const boost::filesystem::path& file, const Options& options);

  ~Store();

  // ---------------------------------------------------------------------------
  // Public thread-safe interface, no external synchronization needed.
  // ---------------------------------------------------------------------------

  template <bool IsMutable>
  std::uint32_t put(const BasicBlock<IsMutable>& block) {
    MT_REQUIRE_EQ(block.size(), getBlockSize());
    std::lock_guard<std::mutex> lock(mutex_);
    return putUnlocked(block.data());
  }

  template <bool IsMutable>
  void put(std::vector<ExtendedBasicBlock<IsMutable> >& blocks) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& block : blocks) {
      if (!block.ignore) {
        MT_REQUIRE_EQ(block.size(), getBlockSize());
        block.id = putUnlocked(block.data());
      }
    }
  }

  void get(std::uint32_t id, ReadWriteBlock& block) const {
    std::lock_guard<std::mutex> lock(mutex_);
    getUnlocked(id, block.data());
  }

  void get(ExtendedReadWriteBlock& block) const {
    std::lock_guard<std::mutex> lock(mutex_);
    getUnlocked(block.id, block.data());
  }

  void get(std::vector<ExtendedReadWriteBlock>& blocks) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& block : blocks) {
      if (!block.ignore) {
        getUnlocked(block.id, block.data());
      }
    }
  }

  template <bool IsMutable>
  void replace(std::uint32_t id, const BasicBlock<IsMutable>& block) {
    MT_REQUIRE_EQ(block.size(), getBlockSize());
    std::lock_guard<std::mutex> lock(mutex_);
    replaceUnlocked(id, block.data());
  }

  template <bool IsMutable>
  void replace(const ExtendedBasicBlock<IsMutable>& block) {
    MT_REQUIRE_EQ(block.size(), getBlockSize());
    std::lock_guard<std::mutex> lock(mutex_);
    replaceUnlocked(block.id, block.data());
  }

  template <bool IsMutable>
  void replace(const std::vector<ExtendedBasicBlock<IsMutable> >& blocks) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& block : blocks) {
      if (!block.ignore) {
        MT_REQUIRE_EQ(block.size(), getBlockSize());
        replaceUnlocked(block.id, block.data());
      }
    }
  }

  enum class AccessPattern { NORMAL, WILLNEED };
  // The names are borrowed from `posix_fadvise`.

  void adviseAccessPattern(AccessPattern pattern) const;

  bool isReadOnly() const { return buffer_.size == 0; }

  std::size_t getBlockSize() const { return stats_.block_size; }

  Stats getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
  }

 private:
  // ---------------------------------------------------------------------------
  // Private non-thread-safe interface, needs external synchronization.
  // ---------------------------------------------------------------------------

  std::uint32_t putUnlocked(const char* block);

  void getUnlocked(std::uint32_t id, char* block) const;

  void replaceUnlocked(std::uint32_t id, const char* block);

  char* getAddressOf(std::uint32_t id) const;

  struct Mapped {
    void* data = nullptr;
    std::size_t size = 0;

    std::size_t num_blocks(std::size_t block_size) const {
      // Callers must ensure that `block_size` isn't zero.
      return size / block_size;
    }
  };

  struct Buffer {
    std::unique_ptr<char[]> data;
    std::size_t offset = 0;
    std::size_t size = 0;

    bool empty() const { return offset == 0; }

    std::size_t num_blocks(std::size_t block_size) const {
      // Callers must ensure that `block_size` isn't zero.
      return size / block_size;
    }
  };

  mutable std::mutex mutex_;
  mutable bool fill_page_cache_ = false;
  mt::AutoCloseFd fd_;
  Mapped mapped_;
  Buffer buffer_;
  Stats stats_;
  bool quiet_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_STORE_HPP_INCLUDED
