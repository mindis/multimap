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

#include "multimap/internal/Store.hpp"

#include <boost/filesystem/operations.hpp>

namespace multimap {
namespace internal {

Store::Store(const boost::filesystem::path& filename, const Options& options)
    : options_(options) {
  MT_REQUIRE_NOT_ZERO(getBlockSize());
  if (boost::filesystem::is_regular_file(filename)) {
    fd_ = mt::open(filename, options.readonly ? O_RDONLY : O_RDWR);
    mt::seek(fd_.get(), 0, SEEK_END);
    const auto length = mt::tell(fd_.get());
    mt::Check::isZero(length % getBlockSize(),
                      "Store: block size does not match size of data file");
    if (length != 0) {
      auto prot = PROT_READ;
      if (!options.readonly) {
        prot |= PROT_WRITE;
      }
      mapped_.data = mt::mmap(nullptr, length, prot, MAP_SHARED, fd_.get(), 0);
      mapped_.size = length;
    }

  } else {
    fd_ = mt::open(filename, O_RDWR | O_CREAT, 0644);
  }
  if (!options.readonly) {
    mt::Check::isZero(options.buffer_size % getBlockSize(),
                      "Store: buffer size must be a multiple of block size");
    buffer_.data.reset(new char[options.buffer_size]);
    buffer_.size = options.buffer_size;
  }
}

Store::~Store() {
  if (fd_.get() != -1) {
    if (mapped_.data) {
      mt::munmap(mapped_.data, mapped_.size);
    }
    if (!buffer_.empty()) {
      mt::write(fd_.get(), buffer_.data.get(), buffer_.offset);
    }
  }
}

void Store::adviseAccessPattern(AccessPattern pattern) const {
  std::lock_guard<std::mutex> lock(mutex_);
  switch (pattern) {
    case AccessPattern::NORMAL:
      fill_page_cache_ = false;
      break;
    case AccessPattern::WILLNEED:
      fill_page_cache_ = true;
      break;
    default:
      MT_FAIL("Default case in switch statement reached");
  }
}

uint32_t Store::putUnlocked(const char* block) {
  if (buffer_.full()) {
    // Flush buffer and remap data file.
    buffer_.flushTo(fd_.get());
    const auto new_size = mt::tell(fd_.get());
    const auto prot = PROT_READ | PROT_WRITE;
    if (mapped_.data) {
      // fsync(fd_);
      // Since Linux provides a so-called unified virtual memory system, it
      // is not necessary to write the content of the buffer cache to disk to
      // ensure that the newly appended data is visible after the remapping.
      // In a unified virtual memory system, memory mappings and blocks of the
      // buffer cache share the same pages of physical memory. [kerrisk p1032]
      MT_ASSERT_LT(mapped_.size, new_size);
#ifdef _GNU_SOURCE
      mapped_.data =
          mt::mremap(mapped_.data, mapped_.size, new_size, MREMAP_MAYMOVE);
#else
      mt::munmap(mapped_.data, mapped_.size);
      mapped_.data =
          mt::mmap(nullptr, new_size, prot, MAP_SHARED, fd_.get(), 0);
#endif
    } else {
      mapped_.data =
          mt::mmap(nullptr, new_size, prot, MAP_SHARED, fd_.get(), 0);
    }
    mapped_.size = new_size;
  }

  std::memcpy(buffer_.data.get() + buffer_.offset, block, getBlockSize());
  buffer_.offset += getBlockSize();

  return getNumBlocksUnlocked() - 1;
}

void Store::getUnlocked(uint32_t id, char* block) const {
  if (fill_page_cache_) {
    fill_page_cache_ = false;
    // Touch each block to load it into the OS page cache.
    const auto num_blocks_mapped = mapped_.getNumBlocks(getBlockSize());
    for (uint32_t i = 0; i != num_blocks_mapped; ++i) {
      std::memcpy(block, getAddressOf(i), getBlockSize());
    }
  }
  std::memcpy(block, getAddressOf(id), getBlockSize());
}

void Store::replaceUnlocked(uint32_t id, const char* block) {
  MT_REQUIRE_NOT_NULL(block);
  std::memcpy(getAddressOf(id), block, getBlockSize());
}

char* Store::getAddressOf(uint32_t id) const {
  MT_REQUIRE_LT(id, getNumBlocksUnlocked());

  const auto num_blocks_mapped = mapped_.getNumBlocks(getBlockSize());
  if (id < num_blocks_mapped) {
    const auto offset = getBlockSize() * id;
    return static_cast<char*>(mapped_.data) + offset;
  } else {
    const auto offset = getBlockSize() * (id - num_blocks_mapped);
    return buffer_.data.get() + offset;
  }
}

}  // namespace internal
}  // namespace multimap
