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

#include "multimap/internal/Store.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <boost/filesystem/operations.hpp>

namespace multimap {
namespace internal {

namespace {

Store::Stats readStatsFromTail(int fd) {
  Store::Stats stats;
  const auto end_of_data = ::lseek64(fd, -(sizeof stats), SEEK_END);
  MT_ASSERT_NE(end_of_data, -1);

  const auto status = ::read(fd, &stats, sizeof stats);
  MT_ASSERT_NE(status, -1);
  MT_ASSERT_EQ(static_cast<std::size_t>(status), sizeof stats);
  return stats;
}

void writeStatsToTail(const Store::Stats& stats, int fd) {
  auto status = ::lseek64(fd, 0, SEEK_END);
  MT_ASSERT_NE(status, -1);

  status = ::write(fd, &stats, sizeof stats);
  MT_ASSERT_NE(status, -1);
  MT_ASSERT_EQ(static_cast<std::size_t>(status), sizeof stats);
}

void removeStatsFromTail(int fd) {
  const auto end_of_data = ::lseek64(fd, -sizeof(Store::Stats), SEEK_CUR);
  MT_ASSERT_NE(end_of_data, -1);
  const auto status = ::ftruncate64(fd, end_of_data);
  MT_ASSERT_ZERO(status);
}

} // namespace

Store::Stats& Store::Stats::combine(const Stats& other) {
  if (block_size == 0) {
    block_size = other.block_size;
  } else {
    MT_ASSERT_EQ(block_size, other.block_size);
  }
  num_blocks += other.num_blocks;
  return *this;
}

Store::Stats Store::Stats::combine(const Stats& a, const Stats& b) {
  return Stats(a).combine(b);
}

Store::Stats Store::Stats::fromProperties(const mt::Properties& properties) {
  Stats stats;
  stats.block_size = std::stoul(properties.at("block_size"));
  stats.num_blocks = std::stoul(properties.at("num_blocks"));
  return stats;
}

mt::Properties Store::Stats::toProperties() const {
  mt::Properties properties;
  properties["block_size"] = std::to_string(block_size);
  properties["num_blocks"] = std::to_string(num_blocks);
  return properties;
}

Store::Store(const boost::filesystem::path& filepath)
    : Store(filepath, Options()) {}

Store::Store(const boost::filesystem::path& filepath, const Options& options) {
  if (boost::filesystem::is_regular_file(filepath)) {
    mt::Check::isFalse(options.error_if_exists, "Store already exists");

    if (options.readonly) {
      fd_ = ::open(filepath.c_str(), O_RDONLY);
      mt::Check::notEqual(
          fd_, -1, "Could not open '%s' in read-only mode because of '%s'",
          filepath.c_str(), std::strerror(errno));
    } else {
      fd_ = ::open(filepath.c_str(), O_RDWR);
      mt::Check::notEqual(
          fd_, -1, "Could not open '%s' in read-write mode because of '%s'",
          filepath.c_str(), std::strerror(errno));
    }

    stats_ = readStatsFromTail(fd_);
    if (!options.readonly) {
      removeStatsFromTail(fd_);
    }

    if (stats_.num_blocks != 0) {
      auto prot = PROT_READ;
      if (!options.readonly) {
        prot |= PROT_WRITE;
      }
      const auto len = stats_.num_blocks * stats_.block_size;
      mapped_.data = ::mmap64(nullptr, len, prot, MAP_SHARED, fd_, 0);
      mt::Check::notEqual(mapped_.data, MAP_FAILED,
                          "mmap64() failed for '%s' because of '%s'",
                          filepath.c_str(), std::strerror(errno));
      mapped_.size = len;
    }

  } else if (options.create_if_missing) {
    MT_REQUIRE_GT(options.buffer_size, options.block_size);
    MT_REQUIRE_ZERO(options.buffer_size % options.block_size);

    if (options.readonly) {
      fd_ = ::open(filepath.c_str(), O_RDONLY | O_CREAT, 0644);
      // Creating a file in read-only mode sounds strange, but we'll try.
      mt::Check::notEqual(
          fd_, -1, "Could not create '%s' in read-only mode because of '%s'",
          filepath.c_str(), std::strerror(errno));
    } else {
      fd_ = ::open(filepath.c_str(), O_RDWR | O_CREAT, 0644);
      mt::Check::notEqual(
          fd_, -1, "Could not create '%s' in read-write mode because of '%s'",
          filepath.c_str(), std::strerror(errno));
    }
    stats_.block_size = options.block_size;

  } else {
    mt::failFormat("Could not open '%s' because it does not exist",
                   filepath.c_str());
  }

  if (!options.readonly) {
    buffer_.data.reset(new char[options.buffer_size]);
    buffer_.size = options.buffer_size;
  }
}

Store::~Store() {
  if (fd_ != -1) {
    if (mapped_.data) {
      const auto status = ::munmap(mapped_.data, mapped_.size);
      mt::Check::isZero(status, "munmap() failed because of '%s'",
                        std::strerror(errno));
    }
    if (!buffer_.empty()) {
      const auto nbytes = ::write(fd_, buffer_.data.get(), buffer_.offset);
      mt::Check::notEqual(nbytes, -1, "write() failed because of '%s'",
                          std::strerror(errno));
      mt::Check::isEqual(static_cast<std::size_t>(nbytes), buffer_.offset,
                         "write() wrote less data than expected");
    }
    if (!isReadOnly()) {
      writeStatsToTail(getStats(), fd_);
    }
    const auto status = ::close(fd_);
    mt::Check::isZero(status, "close() failed because of '%s'",
                      std::strerror(errno));
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

std::uint32_t Store::putUnlocked(const char* block) {
  if (buffer_.offset == buffer_.size) {
    // Flush buffer
    const auto nbytes = ::write(fd_, buffer_.data.get(), buffer_.size);
    mt::Check::notEqual(nbytes, -1, "write() failed because of '%s'",
                        std::strerror(errno));
    mt::Check::isEqual(static_cast<std::size_t>(nbytes), buffer_.size,
                       "write() wrote less data than expected");
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
      mapped_.data =
          ::mremap(mapped_.data, mapped_.size, new_size, MREMAP_MAYMOVE);
      mt::Check::notEqual(mapped_.data, MAP_FAILED,
                          "mremap() failed because of '%s'",
                          std::strerror(errno));
    } else {
      const auto prot = PROT_READ | PROT_WRITE;
      mapped_.data = ::mmap64(nullptr, new_size, prot, MAP_SHARED, fd_, 0);
      mt::Check::notEqual(mapped_.data, MAP_FAILED,
                          "mmap64() failed because of '%s'",
                          std::strerror(errno));
    }
    mapped_.size = new_size;
  }

  std::memcpy(buffer_.data.get() + buffer_.offset, block, stats_.block_size);
  buffer_.offset += stats_.block_size;

  return stats_.num_blocks++;
}

void Store::getUnlocked(std::uint32_t id, char* block) const {
  if (fill_page_cache_) {
    fill_page_cache_ = false;
    // Touch each block to load it into the OS page cache.
    const auto num_blocks_mapped = mapped_.num_blocks(stats_.block_size);
    for (std::uint32_t i = 0; i != num_blocks_mapped; ++i) {
      std::memcpy(block, getAddressOf(i), stats_.block_size);
    }
  }
  std::memcpy(block, getAddressOf(id), stats_.block_size);
}

void Store::replaceUnlocked(std::uint32_t id, const char* block) {
  MT_REQUIRE_NOT_NULL(block);
  std::memcpy(getAddressOf(id), block, stats_.block_size);
}

char* Store::getAddressOf(std::uint32_t id) const {
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
