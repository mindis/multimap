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
#include <aio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
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

Store::Store()
    : num_blocks_written_(0),
      num_blocks_mapped_(0),
      load_factor_total_(0),
      data_(nullptr),
      fd_(-1) {}

Store::Store(const boost::filesystem::path& path, const Options& options)
    : Store() {
  Open(path, options);
}

Store::~Store() {
  try {
    Close();
  } catch (std::exception& error) {
    System::Log("Exception in ~Store") << error.what() << '\n';
  }
}

void Store::Open(const boost::filesystem::path& path, const Options& options) {
  assert(fd_ == -1);
  assert(options.block_size != 0);

  if (boost::filesystem::is_regular_file(path)) {
    fd_ = open(path.c_str(), O_RDWR, 0644);
    Check(fd_ != -1, "Could not open '%s'.", path.c_str());

    const auto fsize = boost::filesystem::file_size(path);
    Check(fsize % options.block_size == 0,
          "Size of '%s' (%u bytes) is not a multiple of block size (%u bytes).",
          path.c_str(), fsize, options.block_size);

    if (fsize != 0) {
      const auto prot = PROT_READ | PROT_WRITE;
      data_ = mmap64(nullptr, fsize, prot, MAP_SHARED, fd_, 0);
      Check(data_ != MAP_FAILED, "mmap64() failed for '%s' because: %s",
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
  path_ = path;
  options_ = options;
}

void Store::Close() {
  if (fd_ != -1) {
    const auto status = close(fd_);
    Check(status != -1, "%s: close() failed.", __PRETTY_FUNCTION__);
    fd_ = -1;
  }
}

std::uint32_t Store::Append(const Block& block) {
  assert(fd_ != -1);
  assert(block.size() == options_.block_size);

  const std::lock_guard<std::mutex> lock(mutex_);
  const auto nbytes = write(fd_, block.data(), block.size());
  assert(nbytes != -1);
  assert(static_cast<std::size_t>(nbytes) == block.size());
  load_factor_total_ += block.load_factor();
  ++num_blocks_written_;

  const auto num_blocks_not_mapped = num_blocks_written_ - num_blocks_mapped_;
  if (num_blocks_not_mapped == options_.remap_every_nblocks) {
    const auto old_size = options_.block_size * num_blocks_mapped_;
    const auto new_size = options_.block_size * num_blocks_written_;
    if (data_) {
      // fsync(fd_);
      // Since Linux provides a so-called unified virtual memory system,
      // it is not necessary to write the content of the buffer cache to disk
      // to ensure that the newly appended data is visible after the remapping.
      // In a unified virtual memory system, memory mappings and blocks of the
      // buffer cache share the same pages of physical memory. [kerrisk p1032]
      data_ = mremap(data_, old_size, new_size, MREMAP_MAYMOVE);
      assert(data_ != MAP_FAILED);
    } else {
      const auto prot = PROT_READ | PROT_WRITE;
      data_ = mmap64(nullptr, new_size, prot, MAP_SHARED, fd_, 0);
      assert(data_ != MAP_FAILED);
    }
    num_blocks_mapped_ = num_blocks_written_;
  }
  return num_blocks_written_ - 1;
}

void Store::Read(std::uint32_t block_id, Block* block, Arena* arena) const {
  assert(fd_ != -1);
  assert(block != nullptr);
  assert(block_id < num_blocks_written_);

  if (block->has_data()) {
    assert(block->size() == options_.block_size);
  } else {
    assert(arena != nullptr);
    block->set_data(arena->Allocate(options_.block_size), options_.block_size);
  }

  const std::lock_guard<std::mutex> lock(mutex_);
  if (block_id < num_blocks_mapped_) {
    std::memcpy(block->data(), address(block_id), block->size());
  } else {
    const auto nbytes =
        pread64(fd_, block->data(), block->size(), offset(block_id));
    assert(nbytes != -1);
    assert(static_cast<std::size_t>(nbytes) == block->size());
  }
}

void Store::Read(std::vector<BlockWithId>* blocks, Arena* arena) const {
  assert(fd_ != -1);
  assert(blocks != nullptr);

  std::vector<aiocb64> aio_list;
  aio_list.reserve(blocks->size());
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    for (auto& block : *blocks) {
      if (!block.ignore) {
        assert(block.id < num_blocks_written_);

        if (block.has_data()) {
          assert(block.size() == options_.block_size);
        } else {
          assert(arena != nullptr);
          const auto block_data = arena->Allocate(options_.block_size);
          block.set_data(block_data, options_.block_size);
        }

        if (block.id < num_blocks_mapped_) {
          std::memcpy(block.data(), address(block.id), block.size());
        } else {
          aio_list.emplace_back();
          aio_list.back().aio_fildes = fd_;
          aio_list.back().aio_lio_opcode = LIO_READ;
          aio_list.back().aio_buf = block.data();
          aio_list.back().aio_nbytes = block.size();
          aio_list.back().aio_offset = offset(block.id);
        }
      }
    }
  }  // End of locked scope.

  if (!aio_list.empty()) {
    std::vector<aiocb64*> aio_ptrs(aio_list.size());
    for (std::size_t i = 0; i != aio_list.size(); ++i) {
      aio_ptrs[i] = &aio_list[i];
    }
    const auto status =
        lio_listio64(LIO_WAIT, aio_ptrs.data(), aio_ptrs.size(), nullptr);
    assert(status != -1);
  }
}

void Store::Write(std::uint32_t block_id, const Block& block) {
  assert(fd_ != -1);
  assert(block_id < num_blocks_written_);
  assert(block.size() == options_.block_size);

  const std::lock_guard<std::mutex> lock(mutex_);
  if (block_id < num_blocks_mapped_) {
    std::memcpy(address(block_id), block.data(), block.size());
  } else {
    const auto nbytes =
        pwrite64(fd_, block.data(), block.size(), offset(block_id));
    assert(nbytes != -1);
    assert(static_cast<std::size_t>(nbytes) == block.size());
  }
}

void Store::Write(const std::vector<BlockWithId>& blocks) {
  assert(fd_ != -1);

  std::vector<aiocb64> aio_list;
  aio_list.reserve(blocks.size());
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& block : blocks) {
      if (!block.ignore) {
        assert(block.id < num_blocks_written_);
        assert(block.size() == options_.block_size);

        if (block.id < num_blocks_mapped_) {
          std::memcpy(address(block.id), block.data(), block.size());
        } else {
          aio_list.emplace_back();
          aio_list.back().aio_fildes = fd_;
          aio_list.back().aio_lio_opcode = LIO_WRITE;
          aio_list.back().aio_buf = const_cast<char*>(block.data());
          aio_list.back().aio_nbytes = block.size();
          aio_list.back().aio_offset = offset(block.id);
        }
      }
    }
  }  // End of locked scope.

  if (!aio_list.empty()) {
    std::vector<aiocb64*> aio_ptrs(aio_list.size());
    for (std::size_t i = 0; i != aio_list.size(); ++i) {
      aio_ptrs[i] = &aio_list[i];
    }
    const auto status =
        lio_listio64(LIO_WAIT, aio_ptrs.data(), aio_ptrs.size(), nullptr);
    assert(status != -1);
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

std::uint64_t Store::offset(std::uint32_t block_id) const {
  return options_.block_size * block_id;
}

char* Store::address(std::uint32_t block_id) const {
  return static_cast<char*>(data_) + offset(block_id);
}

}  // namespace internal
}  // namespace multimap
