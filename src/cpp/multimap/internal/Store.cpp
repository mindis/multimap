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
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include "boost/filesystem/operations.hpp"
#include "multimap/internal/Check.hpp"
#include "multimap/internal/System.hpp"

namespace multimap {
namespace internal {

std::map<std::string, std::string> Store::Stats::ToMap() const {
  return ToMap("");
}

std::map<std::string, std::string> Store::Stats::ToMap(
    const std::string& prefix) const {
  auto prefix_copy = prefix;
  if (!prefix_copy.empty()) {
    prefix_copy.push_back('.');
  }
  std::map<std::string, std::string> map;
  map[prefix_copy + "block_size"] = std::to_string(block_size);
  map[prefix_copy + "num_blocks"] = std::to_string(num_blocks);
  map[prefix_copy + "load_factor_avg"] = std::to_string(load_factor_avg);
  //  map[prefix_copy + "load_factor_max"] = std::to_string(load_factor_max);
  //  map[prefix_copy + "load_factor_min"] = std::to_string(load_factor_min);
  return map;
}

Store::Store(const boost::filesystem::path& path, const Options& options)
    : Store() {
  Open(path, options);
}

Store::~Store() { Close(); }

void Store::Open(const boost::filesystem::path& path, const Options& options) {
  assert(fd_ == -1);
  assert(options.block_size != 0);
  assert(options.buffer_size > options.block_size);
  assert(options.buffer_size % options.block_size == 0);

  if (boost::filesystem::is_regular_file(path)) {
    fd_ = open(path.c_str(), O_RDWR, 0644);
    Check(fd_ != -1, "Could not open '%s'.", path.c_str());

    const auto fsize = boost::filesystem::file_size(path);
    Check(fsize % options.block_size == 0,
          "Size of '%s' (%u bytes) is not a multiple of block size (%u bytes).",
          path.c_str(), fsize, options.block_size);

    if (fsize != 0) {
      const auto prot = PROT_READ | PROT_WRITE;
      mmap_addr_ = mmap64(nullptr, fsize, prot, MAP_SHARED, fd_, 0);
      Check(mmap_addr_ != MAP_FAILED, "mmap64() failed for '%s' because: %s",
            path.c_str(), std::strerror(errno));
    }

    num_blocks_written_ = fsize / options.block_size;
    num_blocks_mapped_ = num_blocks_written_;

  } else if (options.create_if_missing) {
    fd_ = open(path.c_str(), O_RDWR | O_CREAT, 0644);
    Check(fd_ != -1, "Could not create '%s'.", path.c_str());
  } else {
    Throw("Does not exist: '%s'.", path.c_str());
  }
  buffer_.reset(new char[options.buffer_size]);
  options_ = options;
  path_ = path;
}

void Store::Close() {
  const std::lock_guard<std::mutex> lock(mutex_);
  if (mmap_addr_) {
    const auto mmap_len = options_.block_size * num_blocks_mapped_;
    const auto status = munmap(mmap_addr_, mmap_len);
    assert(status == 0);
  }
  if (buffer_ && buffer_offset_ != 0) {
    const auto nbytes = write(fd_, buffer_.get(), buffer_offset_);
    assert(nbytes != -1);
    assert(static_cast<std::size_t>(nbytes) == buffer_offset_);
  }
  if (fd_ != -1) {
    const auto status = close(fd_);
    assert(status == 0);
  }
  buffer_.reset();
  num_blocks_written_ = 0;
  num_blocks_mapped_ = 0;
  load_factor_total_ = 0;
  buffer_offset_ = 0;
  mmap_addr_ = nullptr;
  fd_ = -1;
}

std::uint32_t Store::Append(const Block& block) {
  assert(fd_ != -1);
  assert(block.size() == options_.block_size);

  const std::lock_guard<std::mutex> lock(mutex_);
  if (buffer_offset_ == options_.buffer_size) {
    // Flush buffer
    const auto nbytes = write(fd_, buffer_.get(), options_.buffer_size);
    assert(nbytes != -1);
    assert(static_cast<std::size_t>(nbytes) == options_.buffer_size);
    buffer_offset_ = 0;

    // Remap file
    const auto old_size = options_.block_size * num_blocks_mapped_;
    const auto new_size = options_.block_size * num_blocks_written_;
    if (mmap_addr_) {
      // fsync(fd_);
      // Since Linux provides a so-called unified virtual memory system,
      // it is not necessary to write the content of the buffer cache to disk
      // to ensure that the newly appended data is visible after the remapping.
      // In a unified virtual memory system, memory mappings and blocks of the
      // buffer cache share the same pages of physical memory. [kerrisk p1032]
      mmap_addr_ = mremap(mmap_addr_, old_size, new_size, MREMAP_MAYMOVE);
      assert(mmap_addr_ != MAP_FAILED);
    } else {
      const auto prot = PROT_READ | PROT_WRITE;
      mmap_addr_ = mmap64(nullptr, new_size, prot, MAP_SHARED, fd_, 0);
      assert(mmap_addr_ != MAP_FAILED);
    }
    num_blocks_mapped_ = num_blocks_written_;
  }

  std::memcpy(buffer_.get() + buffer_offset_, block.data(), block.size());
  buffer_offset_ += block.size();

  load_factor_total_ += block.load_factor();
  return num_blocks_written_++;
}

void Store::Read(std::uint32_t block_id, Block* block, Arena* arena) const {
  assert(fd_ != -1);
  assert(block != nullptr);

  if (block->has_data()) {
    assert(block->size() == options_.block_size);
  } else {
    assert(arena != nullptr);
    block->set_data(arena->Allocate(options_.block_size), options_.block_size);
  }

  const std::lock_guard<std::mutex> lock(mutex_);
  std::memcpy(block->data(), AddressOf(block_id), block->size());
}

void Store::Read(std::vector<BlockWithId>* blocks, Arena* arena) const {
  assert(fd_ != -1);
  assert(blocks != nullptr);

  const std::lock_guard<std::mutex> lock(mutex_);
  for (auto& block : *blocks) {
    if (block.ignore) continue;

    if (block.has_data()) {
      assert(block.size() == options_.block_size);
    } else {
      assert(arena != nullptr);
      const auto block_data = arena->Allocate(options_.block_size);
      block.set_data(block_data, options_.block_size);
    }

    std::memcpy(block.data(), AddressOf(block.id), block.size());
  }
}

void Store::Write(std::uint32_t block_id, const Block& block) {
  assert(fd_ != -1);
  assert(block.size() == options_.block_size);

  const std::lock_guard<std::mutex> lock(mutex_);
  std::memcpy(AddressOf(block_id), block.data(), block.size());
}

void Store::Write(const std::vector<BlockWithId>& blocks) {
  assert(fd_ != -1);

  const std::lock_guard<std::mutex> lock(mutex_);
  for (const auto& block : blocks) {
    if (block.ignore) continue;
    assert(block.size() == options_.block_size);

    std::memcpy(AddressOf(block.id), block.data(), block.size());
  }
}

// https://bitbucket.org/mtrenkmann/multimap/issues/11
Store::Stats Store::GetStats() const {
  const std::lock_guard<std::mutex> lock(mutex_);
  Stats stats;
  stats.block_size = options_.block_size;
  stats.num_blocks = num_blocks_written_;
  if (stats.num_blocks != 0) {
    stats.load_factor_avg = load_factor_total_ / num_blocks_written_;
    //    stats.load_factor_min = TODO;
    //    stats.load_factor_max = TODO;
  }
  return stats;
}

const boost::filesystem::path& Store::path() const {
  // Is read-only after Open and needs no locking.
  return path_;
}

const Store::Options& Store::options() const {
  // Is read-only after Open and needs no locking.
  return options_;
}

char* Store::AddressOf(std::uint32_t block_id) const {
  assert(block_id < num_blocks_written_);
  if (block_id < num_blocks_mapped_) {
    const auto offset = options_.block_size * block_id;
    return static_cast<char*>(mmap_addr_) + offset;
  } else {
    const auto offset = options_.block_size * (block_id - num_blocks_mapped_);
    return buffer_.get() + offset;
  }
}

}  // namespace internal
}  // namespace multimap
