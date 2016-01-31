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

namespace {

Store::Stats readStatsFromTail(int fd) {
  Store::Stats stats;
  mt::seek(fd, -(sizeof stats), SEEK_END);
  mt::read(fd, &stats, sizeof stats);
  return stats;
}

void writeStatsToTail(const Store::Stats& stats, int fd, bool quiet) {
  mt::seek(fd, 0, SEEK_END);
  if (quiet) {
    mt::write(fd, &stats, sizeof stats);
  } else {
    mt::writeOrPrompt(fd, &stats, sizeof stats);
  }
}

void removeStatsFromTail(int fd) {
  const auto end_of_data = mt::seek(fd, -sizeof(Store::Stats), SEEK_CUR);
  mt::truncate(fd, end_of_data);
}

} // namespace

Store::Stats Store::Stats::fromProperties(const mt::Properties& properties) {
  Stats stats;
  stats.block_size = std::stoull(properties.at("block_size"));
  stats.num_blocks = std::stoull(properties.at("num_blocks"));
  return stats;
}

mt::Properties Store::Stats::toProperties() const {
  mt::Properties properties;
  properties["block_size"] = std::to_string(block_size);
  properties["num_blocks"] = std::to_string(num_blocks);
  return properties;
}

Store::Store(const boost::filesystem::path& file) : Store(file, Options()) {}

Store::Store(const boost::filesystem::path& file, const Options& options) {
  if (boost::filesystem::is_regular_file(file)) {
    fd_ = mt::open(file, options.readonly ? O_RDONLY : O_RDWR);
    stats_ = readStatsFromTail(fd_.get());
    if (!options.readonly) {
      removeStatsFromTail(fd_.get());
    }
    if (stats_.num_blocks) {
      auto prot = PROT_READ;
      if (!options.readonly) {
        prot |= PROT_WRITE;
      }
      const auto len = stats_.num_blocks * stats_.block_size;
      mapped_.data = mt::mmap(nullptr, len, prot, MAP_SHARED, fd_.get(), 0);
      mapped_.size = len;
    }

  } else {
    // Create new data file.
    mt::Check::isZero(
        options.buffer_size % options.block_size,
        "options.buffer_size must be a multiple of options.block_size");

    fd_ = mt::open(file, O_RDWR | O_CREAT, 0644);
    stats_.block_size = options.block_size;
  }
  if (!options.readonly) {
    buffer_.data.reset(new char[options.buffer_size]);
    buffer_.size = options.buffer_size;
  }
  quiet_ = options.quiet;
}

Store::~Store() {
  if (fd_.get() != -1) {
    if (mapped_.data) {
      mt::munmap(mapped_.data, mapped_.size);
    }
    if (!buffer_.empty()) {
      if (quiet_) {
        mt::write(fd_.get(), buffer_.data.get(), buffer_.offset);
      } else {
        mt::writeOrPrompt(fd_.get(), buffer_.data.get(), buffer_.offset);
      }
    }
    if (!isReadOnly()) {
      writeStatsToTail(getStats(), fd_.get(), quiet_);
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
      mt::fail("Default case in switch statement reached");
  }
}

uint32_t Store::putUnlocked(const char* block) {
  if (buffer_.offset == buffer_.size) {
    // Flush buffer
    if (quiet_) {
      mt::write(fd_.get(), buffer_.data.get(), buffer_.size);
    } else {
      mt::writeOrPrompt(fd_.get(), buffer_.data.get(), buffer_.size);
    }
    buffer_.offset = 0;

    // Remap file
    const auto new_size = stats_.block_size * stats_.num_blocks;
    if (mapped_.data) {
      // fsync(fd_);
      // Since Linux provides a so-called unified virtual memory system, it
      // is not necessary to write the content of the buffer cache to disk to
      // ensure that the newly appended data is visible after the remapping.
      // In a unified virtual memory system, memory mappings and blocks of the
      // buffer cache share the same pages of physical memory. [kerrisk p1032]
      MT_ASSERT_LT(mapped_.size, new_size);
      mapped_.data =
          mt::mremap(mapped_.data, mapped_.size, new_size, MREMAP_MAYMOVE);
    } else {
      const auto prot = PROT_READ | PROT_WRITE;
      mapped_.data =
          mt::mmap(nullptr, new_size, prot, MAP_SHARED, fd_.get(), 0);
    }
    mapped_.size = new_size;
  }

  std::memcpy(buffer_.data.get() + buffer_.offset, block, stats_.block_size);
  buffer_.offset += stats_.block_size;

  return stats_.num_blocks++;
}

void Store::getUnlocked(uint32_t id, char* block) const {
  if (fill_page_cache_) {
    fill_page_cache_ = false;
    // Touch each block to load it into the OS page cache.
    const auto num_blocks_mapped = mapped_.num_blocks(stats_.block_size);
    for (uint32_t i = 0; i != num_blocks_mapped; ++i) {
      std::memcpy(block, getAddressOf(i), stats_.block_size);
    }
  }
  std::memcpy(block, getAddressOf(id), stats_.block_size);
}

void Store::replaceUnlocked(uint32_t id, const char* block) {
  MT_REQUIRE_NOT_NULL(block);
  std::memcpy(getAddressOf(id), block, stats_.block_size);
}

char* Store::getAddressOf(uint32_t id) const {
  MT_REQUIRE_LT(id, stats_.num_blocks);

  const auto num_blocks_mapped = mapped_.num_blocks(stats_.block_size);
  if (id < num_blocks_mapped) {
    const auto offset = stats_.block_size * id;
    return static_cast<char*>(mapped_.data) + offset;
  } else {
    const auto offset = stats_.block_size * (id - num_blocks_mapped);
    return buffer_.data.get() + offset;
  }
}

} // namespace internal
} // namespace multimap
