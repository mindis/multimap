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
#include <sys/types.h>
#include <sys/stat.h>
#include <boost/filesystem/operations.hpp>

namespace multimap {
namespace internal {

namespace {

const char* STATS_FILE_SUFFIX = ".properties";
const char* STORE_FILE_SUFFIX = ".store";

Store::Stats readStatsFromFile(const boost::filesystem::path& file) {
  Store::Stats stats;
  auto properties = mt::readPropertiesFromFile(file.c_str());
  stats.block_size = std::stoul(properties.at("block_size"));
  stats.num_blocks = std::stoul(properties.at("num_blocks"));
  stats.load_factor_min = std::stod(properties.at("load_factor_min"));
  stats.load_factor_max = std::stod(properties.at("load_factor_max"));
  stats.load_factor_avg = std::stod(properties.at("load_factor_avg"));
  return stats;
}

}  // namespace

void Store::Stats::combine(const Stats& other) {
  MT_REQUIRE_LE(load_factor_avg, 1.0);
  MT_REQUIRE_LE(load_factor_max, 1.0);
  MT_REQUIRE_LE(load_factor_min, 1.0);

  if (block_size == 0) {
    block_size = other.block_size;
  } else {
    MT_ASSERT_EQ(block_size, other.block_size);
  }

  if (other.num_blocks != 0) {
    const double total = num_blocks + other.num_blocks;
    load_factor_avg *= (num_blocks / total);
    load_factor_avg += (other.num_blocks / total) * other.load_factor_avg;
    // new_avg = (weight * avg'old) + (weight'other * avg'other)
    load_factor_max = std::max(load_factor_max, other.load_factor_max);
    load_factor_min = std::min(load_factor_min, other.load_factor_min);
    num_blocks += other.num_blocks;
  }
}

std::map<std::string, std::string> Store::Stats::toMap() const {
  return toMap("");
}

std::map<std::string, std::string> Store::Stats::toMap(
    const std::string& prefix) const {
  auto prefix_copy = prefix;
  if (!prefix_copy.empty()) {
    prefix_copy.push_back('.');
  }
  std::map<std::string, std::string> map;
  map[prefix_copy + "block_size"] = std::to_string(block_size);
  map[prefix_copy + "num_blocks"] = std::to_string(num_blocks);
  map[prefix_copy + "load_factor_avg"] = std::to_string(load_factor_avg);
  map[prefix_copy + "load_factor_max"] = std::to_string(load_factor_max);
  map[prefix_copy + "load_factor_min"] = std::to_string(load_factor_min);
  return map;
}

Store::~Store() {
  if (mapped_.data) {
    const auto status = ::munmap(mapped_.data, mapped_.size);
    MT_ASSERT_EQ(status, 0);
  }
  if (buffer_.offset != 0) {
    const auto nbytes = ::write(fd_, buffer_.data.get(), buffer_.offset);
    MT_ASSERT_NE(nbytes, -1);
    MT_ASSERT_EQ(static_cast<std::size_t>(nbytes), buffer_.offset);
  }
  if (fd_ != -1) {
    const auto status = ::close(fd_);
    MT_ASSERT_EQ(status, 0);
  }
  if (!prefix_.empty()) {
    const auto path_to_stats = prefix_.string() + STATS_FILE_SUFFIX;
    mt::writePropertiesToFile(getStats().toMap(), path_to_stats.c_str());
  }
}

Store::Store(const boost::filesystem::path& prefix, std::size_t buffer_size) {
  open(prefix, buffer_size);
}

Store::Store(const boost::filesystem::path& prefix, std::size_t block_size,
             std::size_t buffer_size) {
  create(prefix, block_size, buffer_size);
}

void Store::open(const boost::filesystem::path& prefix,
                 std::size_t buffer_size) {
  MT_REQUIRE_EQ(fd_, -1);

  const auto path_to_stats = prefix.string() + STATS_FILE_SUFFIX;
  const auto path_to_store = prefix.string() + STORE_FILE_SUFFIX;
  MT_REQUIRE_TRUE(boost::filesystem::is_regular_file(path_to_stats));
  MT_REQUIRE_TRUE(boost::filesystem::is_regular_file(path_to_store));

  stats_ = readStatsFromFile(path_to_stats);
  stats_.load_factor_avg *= stats_.num_blocks;
  // Will be normalized back in `getStats()`.

  MT_ASSERT_GT(buffer_size, stats_.block_size);
  MT_ASSERT_EQ(buffer_size % stats_.block_size, 0);

  fd_ = ::open(path_to_store.c_str(), O_RDWR);
  mt::check(fd_ != -1, "Could not open '%s' in read-write mode",
            path_to_store.c_str());

  const auto file_size = boost::filesystem::file_size(path_to_store);
  if (file_size != 0) {
    const auto prot = PROT_READ | PROT_WRITE;
    mapped_.data = ::mmap64(nullptr, file_size, prot, MAP_SHARED, fd_, 0);
    mt::check(mapped_.data != MAP_FAILED,
              "POSIX mmap64() failed for '%s' because: %s",
              path_to_store.c_str(), std::strerror(errno));
    mapped_.size = file_size;
  }

  MT_ASSERT_EQ(file_size % stats_.block_size, 0);
  const auto num_blocks_expected = file_size / stats_.block_size;
  MT_ASSERT_EQ(stats_.num_blocks, num_blocks_expected);

  buffer_.data.reset(new char[buffer_size]);
  buffer_.size = buffer_size;
  prefix_ = prefix;
}

void Store::create(const boost::filesystem::path& prefix,
                   std::size_t block_size, std::size_t buffer_size) {
  MT_REQUIRE_EQ(fd_, -1);
  MT_REQUIRE_GT(buffer_size, block_size);
  MT_REQUIRE_EQ(buffer_size % block_size, 0);
  stats_.block_size = block_size;

  const auto path_to_stats = prefix.string() + STATS_FILE_SUFFIX;
  const auto path_to_store = prefix.string() + STORE_FILE_SUFFIX;
  MT_REQUIRE_FALSE(boost::filesystem::is_regular_file(path_to_stats));
  MT_REQUIRE_FALSE(boost::filesystem::is_regular_file(path_to_store));

  const auto permissions = 0644;
  fd_ = ::open(path_to_store.c_str(), O_CREAT | O_RDWR, permissions);
  mt::check(fd_ != -1, "Could not create '%s' in read-write mode",
            path_to_store.c_str());

  buffer_.data.reset(new char[buffer_size]);
  buffer_.size = buffer_size;
  prefix_ = prefix;
}

std::uint32_t Store::append(const Block& block) {
  MT_REQUIRE_NE(fd_, -1);
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

std::size_t Store::block_size() const { return stats_.block_size; }

Store::Stats Store::getStats() const {
  const std::lock_guard<std::mutex> lock(mutex_);
  Stats result = stats_;
  if (stats_.num_blocks != 0) {
    result.load_factor_avg /= stats_.num_blocks;
  }
  return result;
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

}  // namespace internal
}  // namespace multimap
