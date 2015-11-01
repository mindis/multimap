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
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <boost/filesystem/operations.hpp>

namespace multimap {
namespace internal {

namespace {

Store::Stats readAndRemoveStatsFromTail(int fd) {
  Store::Stats stats;
  auto end_of_data = ::lseek64(fd, -(sizeof stats), SEEK_END);
  MT_ASSERT_NE(end_of_data, -1);

  auto status = ::read(fd, &stats, sizeof stats);
  MT_ASSERT_NE(status, -1);
  MT_ASSERT_EQ(static_cast<std::size_t>(status), sizeof stats);

  end_of_data = ::lseek64(fd, -(sizeof stats), SEEK_CUR);
  MT_ASSERT_NE(end_of_data, -1);

  status = ::ftruncate64(fd, end_of_data);
  MT_ASSERT_IS_ZERO(status);

  return stats;
}

void writeStatsToTail(const Store::Stats& stats, int fd) {
  auto status = ::lseek64(fd, 0, SEEK_END);
  MT_ASSERT_NE(status, -1);

  status = ::write(fd, &stats, sizeof stats);
  MT_ASSERT_NE(status, -1);
  MT_ASSERT_EQ(static_cast<std::size_t>(status), sizeof stats);
}

} // namespace

Store::Stats& Store::Stats::combine(const Stats& other) {
  MT_REQUIRE_LE(load_factor_min, 1.0);
  MT_REQUIRE_LE(load_factor_max, 1.0);
  MT_REQUIRE_LE(load_factor_avg, 1.0);

  if (block_size == 0) {
    block_size = other.block_size;
  } else {
    MT_ASSERT_EQ(block_size, other.block_size);
  }

  if (other.num_blocks != 0) {
    load_factor_min = std::min(load_factor_min, other.load_factor_min);
    load_factor_max = std::max(load_factor_max, other.load_factor_max);
    const double total_blocks = num_blocks + other.num_blocks;
    load_factor_avg =
        ((num_blocks / total_blocks) * load_factor_avg) +
        ((other.num_blocks / total_blocks) * other.load_factor_avg);
    // new_avg = (weight * avg'old) + (weight'other * avg'other)
    num_blocks += other.num_blocks;
  }
  return *this;
}

Store::Stats Store::Stats::combine(const Stats& a, const Stats& b) {
  return Stats(a).combine(b);
}

Store::Stats Store::Stats::fromProperties(const mt::Properties& properties) {
  return fromProperties(properties, "");
}

Store::Stats Store::Stats::fromProperties(const mt::Properties& properties,
                                          const std::string& prefix) {
  auto pfx = prefix;
  if (!pfx.empty()) {
    pfx.push_back('.');
  }
  Stats stats;
  stats.block_size = std::stoul(properties.at(pfx + "block_size"));
  stats.num_blocks = std::stoul(properties.at(pfx + "num_blocks"));
  stats.load_factor_min = std::stod(properties.at(pfx + "load_factor_min"));
  stats.load_factor_max = std::stod(properties.at(pfx + "load_factor_max"));
  stats.load_factor_avg = std::stod(properties.at(pfx + "load_factor_avg"));
  return stats;
}

mt::Properties Store::Stats::toProperties() const { return toProperties(""); }

mt::Properties Store::Stats::toProperties(const std::string& prefix) const {
  auto pfx = prefix;
  if (!pfx.empty()) {
    pfx.push_back('.');
  }
  mt::Properties properties;
  properties[pfx + "block_size"] = std::to_string(block_size);
  properties[pfx + "num_blocks"] = std::to_string(num_blocks);
  properties[pfx + "load_factor_min"] = std::to_string(load_factor_min);
  properties[pfx + "load_factor_max"] = std::to_string(load_factor_max);
  properties[pfx + "load_factor_avg"] = std::to_string(load_factor_avg);
  return properties;
}

Store::~Store() {
  if (isOpen()) {
    if (mapped_.data) {
      const auto status = ::munmap(mapped_.data, mapped_.size);
      MT_ASSERT_EQ(status, 0);
    }
    if (!buffer_.empty()) {
      const auto nbytes = ::write(fd_, buffer_.data.get(), buffer_.offset);
      MT_ASSERT_NE(nbytes, -1);
      MT_ASSERT_EQ(static_cast<std::size_t>(nbytes), buffer_.offset);
    }
    writeStatsToTail(getStats(), fd_);
    const auto status = ::close(fd_);
    MT_ASSERT_EQ(status, 0);
  }
}

Store::Store(const boost::filesystem::path& file, std::size_t block_size,
             std::size_t buffer_size) {
  open(file, block_size, buffer_size);
}

void Store::open(const boost::filesystem::path& file, std::size_t block_size,
                 std::size_t buffer_size) {
  MT_REQUIRE_FALSE(isOpen());

  if (boost::filesystem::is_regular_file(file)) {
    fd_ = ::open(file.c_str(), O_RDWR);
    mt::check(fd_ != -1, "Could not open '%s' in O_RDWR mode", file.c_str());

    stats_ = readAndRemoveStatsFromTail(fd_);
    const auto file_size = boost::filesystem::file_size(file);
    MT_ASSERT_IS_ZERO(file_size % stats_.block_size);
    MT_ASSERT_EQ(file_size / stats_.block_size, stats_.num_blocks);

    stats_.load_factor_avg *= stats_.num_blocks;
    // Will be normalized back in getStats().

    if (file_size != 0) {
      const auto prot = PROT_READ | PROT_WRITE;
      mapped_.data = ::mmap64(nullptr, file_size, prot, MAP_SHARED, fd_, 0);
      mt::check(mapped_.data != MAP_FAILED,
                "POSIX mmap64() failed for '%s' because: %s", file.c_str(),
                std::strerror(errno));
      mapped_.size = file_size;
    }

  } else if (block_size == 0) {
    mt::throwRuntimeError2("Store file '%s' does not exist", file.c_str());

  } else {
    MT_REQUIRE_GT(buffer_size, block_size);
    MT_REQUIRE_IS_ZERO(buffer_size % block_size);

    fd_ = ::open(file.c_str(), O_RDWR | O_CREAT, 0644);
    mt::check(fd_ != -1, "Could not create '%s' in O_RDWR mode", file.c_str());

    stats_.block_size = block_size;
  }

  buffer_.data.reset(new char[buffer_size]);
  buffer_.size = buffer_size;

  MT_ENSURE_TRUE(isOpen());
}

bool Store::isOpen() const { return fd_ != -1; }

std::uint32_t Store::append(const Block& block) {
  MT_REQUIRE_TRUE(isOpen());
  MT_REQUIRE_EQ(block.size(), stats_.block_size);

  const std::lock_guard<std::mutex> lock(mutex_);
  if (buffer_.offset == buffer_.size) {
    // Flush buffer
    const auto nbytes = ::write(fd_, buffer_.data.get(), buffer_.size);
    mt::check(nbytes != -1, "POSIX write() failed");
    mt::check(static_cast<std::size_t>(nbytes) == buffer_.size,
              "POSIX write() wrote only %u of %u bytes", nbytes, buffer_.size);
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
      mt::check(mapped_.data != MAP_FAILED, "POSIX mremap() failed");
    } else {
      const auto prot = PROT_READ | PROT_WRITE;
      mapped_.data = ::mmap64(nullptr, new_size, prot, MAP_SHARED, fd_, 0);
      mt::check(mapped_.data != MAP_FAILED, "POSIX mmap64() failed");
    }
    mapped_.size = new_size;
  }

  std::memcpy(buffer_.data.get() + buffer_.offset, block.data(), block.size());
  buffer_.offset += block.size();

  const auto load_factor = block.load_factor();
  stats_.load_factor_avg += load_factor;
  stats_.load_factor_max = std::max(load_factor, stats_.load_factor_max);
  stats_.load_factor_min = std::min(load_factor, stats_.load_factor_min);

  return stats_.num_blocks++;
}

void Store::read(std::uint32_t id, Block* block, Arena* arena) const {
  MT_REQUIRE_NE(fd_, -1);
  MT_REQUIRE_NOT_NULL(block);

  if (block->hasData()) {
    MT_ASSERT_EQ(block->size(), stats_.block_size);
  } else {
    MT_ASSERT_NOT_NULL(arena);
    block->setData(arena->allocate(stats_.block_size), stats_.block_size);
  }

  const std::lock_guard<std::mutex> lock(mutex_);
  if (fill_page_cache_) {
    fill_page_cache_ = false;
    // Touch each block to load it into the OS page cache.
    std::vector<char> buffer(stats_.block_size);
    const auto num_blocks_mapped = mapped_.num_blocks(stats_.block_size);
    for (std::uint32_t i = 0; i != num_blocks_mapped; ++i) {
      std::memcpy(buffer.data(), getAddressOf(i), stats_.block_size);
    }
  }
  std::memcpy(block->data(), getAddressOf(id), block->size());
}

void Store::write(std::uint32_t id, const Block& block) {
  MT_REQUIRE_NE(fd_, -1);
  MT_REQUIRE_EQ(block.size(), stats_.block_size);

  const std::lock_guard<std::mutex> lock(mutex_);
  std::memcpy(getAddressOf(id), block.data(), block.size());
}

void Store::adviseAccessPattern(AccessPattern pattern) const {
  switch (pattern) {
    case AccessPattern::RANDOM:
      fill_page_cache_ = false;
      break;
    case AccessPattern::SEQUENTIAL:
      fill_page_cache_ = true;
      break;
    default:
      throw "The unexpected happened";
  }
}

Store::Stats Store::getStats() const {
  const std::lock_guard<std::mutex> lock(mutex_);
  Stats result = stats_;
  if (stats_.num_blocks != 0) {
    result.load_factor_avg /= stats_.num_blocks;
  }
  return result;
}

std::size_t Store::block_size() const { return stats_.block_size; }

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
