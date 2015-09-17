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

#include "multimap/internal/BlockStore.hpp"

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
#include "multimap/internal/thirdparty/mt.hpp"

namespace multimap {
namespace internal {

namespace {

const char* NAME_OF_BLOCK_FILE = "multimap";

std::vector<boost::filesystem::path> getPathsOfExistingBlockFiles(
    const boost::filesystem::path& directory) {
  std::vector<boost::filesystem::path> paths;
  boost::filesystem::path filename = NAME_OF_BLOCK_FILE;
  for (std::size_t i = 0; true; ++i) {
    filename.replace_extension('.' + std::to_string(i));
    const auto filepath = directory / filename;
    if (boost::filesystem::is_regular_file(filepath)) {
      paths.push_back(filepath);
    } else {
      break;
    }
  }
  return paths;
}

}  // namespace

std::map<std::string, std::string> BlockStore::Stats::toMap() const {
  return toMap("");
}

std::map<std::string, std::string> BlockStore::Stats::toMap(
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

BlockStore::BlockStore(const boost::filesystem::path& path,
                       const Options& options) {
  open(path, options);
}

void BlockStore::open(const boost::filesystem::path& directory,
                      const Options& options) {
  MT_REQUIRE_TRUE(boost::filesystem::is_directory(directory));
  MT_REQUIRE_GT(options.block_size, 0);
  MT_REQUIRE_GT(options.buffer_size, options.block_size);
  MT_REQUIRE_EQ(options.buffer_size % options.block_size, 0);

  const auto paths = getPathsOfExistingBlockFiles(directory);
  if (paths.empty()) {
    const auto num_block_files = mt::nextPrime(options.num_files);
    MT_ASSERT_LT(num_block_files, 1 << 8);
    block_files_.resize(num_block_files);
    boost::filesystem::path filename = NAME_OF_BLOCK_FILE;
    for (std::size_t i = 0; i != block_files_.size(); ++i) {
      filename.replace_extension('.' + std::to_string(i));
      block_files_[i].reset(new DataFile());
      block_files_[i]->open(directory / filename, options);
    }
  } else {
    block_files_.resize(paths.size());
    for (std::size_t i = 0; i != block_files_.size(); ++i) {
      block_files_[i].reset(new DataFile());
      block_files_[i]->open(paths[i], options);
    }
  }
}

void BlockStore::close() {
  const std::lock_guard<std::mutex> lock(mutex_);
  block_files_.clear();
}

std::uint32_t BlockStore::put(const Bytes& key, const Block& block) {
  MT_REQUIRE_TRUE(block.hasData());

  const std::uint32_t file_id = getFileId(key);
  const std::uint32_t block_id = block_files_[file_id]->append(block);
  MT_ASSERT_LT(block_id, 1 << 24);
  return (file_id << 24) + block_id;
}

void BlockStore::get(std::uint32_t id, Block* block, Arena* arena) const {
  const std::uint32_t file_id = id >> 24;
  const std::uint32_t block_id = id & 0x00FFFFFF;
  MT_ASSERT_LT(file_id, block_files_.size());
  block_files_[file_id]->read(block_id, block, arena);
}

void BlockStore::get(std::vector<BlockWithId>* blocks, Arena* arena) const {
  MT_REQUIRE_NOT_NULL(blocks);

  for (auto& block : *blocks) {
    if (block.ignore) continue;
    get(block.id, &block, arena);
  }
}

void BlockStore::replace(std::uint32_t id, const Block& block) {
  const std::uint32_t file_id = id >> 24;
  const std::uint32_t block_id = id & 0x00FFFFFF;
  MT_ASSERT_LT(file_id, block_files_.size());
  block_files_[file_id]->write(block_id, block);
}

void BlockStore::replace(const std::vector<BlockWithId>& blocks) {
  for (const auto& block : blocks) {
    if (block.ignore) continue;
    replace(block.id, block);
  }
}

std::uint32_t BlockStore::getFileId(const Bytes& key) const {
  MT_REQUIRE_FALSE(key.empty());
  return mt::fnv1aHash32(key.data(), key.size()) % block_files_.size();
}

// https://bitbucket.org/mtrenkmann/multimap/issues/11
BlockStore::Stats BlockStore::getStats() const {
  Stats total_stats;
  total_stats.load_factor_min = -1;
  for (const auto& data_file : block_files_) {
    const auto stats = data_file->getStats();
    total_stats.block_size = stats.block_size;
    total_stats.num_blocks += stats.num_blocks;
    total_stats.load_factor_min =
        std::min(stats.load_factor_min, total_stats.load_factor_min);
    total_stats.load_factor_max =
        std::max(stats.load_factor_max, total_stats.load_factor_max);
    total_stats.load_factor_avg += stats.load_factor_avg;
  }
  if (block_files_.size() != 0) {
    total_stats.load_factor_avg /= block_files_.size();
  }
  return total_stats;
}

const boost::filesystem::path& BlockStore::directory() const {
  return directory_;
}

const BlockStore::Options& BlockStore::options() const { return options_; }

std::size_t BlockStore::num_files() const { return block_files_.size(); }

void BlockStore::advise_access_pattern(AccessPattern pattern) const {
  const std::lock_guard<std::mutex> lock(mutex_);
  for (const auto& block_file : block_files_) {
    block_file->advise_access_pattern(pattern);
  }
}

BlockStore::DataFile::~DataFile() {
  const std::lock_guard<std::mutex> lock(mutex_);
  if (mmap_addr_) {
    const auto mmap_len = options_.block_size * num_blocks_mapped_;
    const auto status = munmap(mmap_addr_, mmap_len);
    assert(status == 0);
  }
  if (buffer_ && buffer_offset_ != 0) {
    const auto nbytes = ::write(fd_, buffer_.get(), buffer_offset_);
    assert(nbytes != -1);
    assert(static_cast<std::size_t>(nbytes) == buffer_offset_);
  }
  if (fd_ != -1) {
    const auto status = ::close(fd_);
    assert(status == 0);
  }
  buffer_.reset();
  num_blocks_written_ = 0;
  num_blocks_mapped_ = 0;
  load_factor_total_ = 0;
  buffer_offset_ = 0;
  mmap_addr_ = nullptr;
  fd_ = -1;
  path_.clear();
  options_ = Options();
}

BlockStore::DataFile::DataFile(const boost::filesystem::path& path,
                               const Options& options) {
  open(path, options);
}

void BlockStore::DataFile::open(const boost::filesystem::path& path,
                                const Options& options) {
  MT_REQUIRE_EQ(fd_, -1);
  MT_REQUIRE_NE(options.block_size, 0);
  MT_REQUIRE_GT(options.buffer_size, options.block_size);
  MT_REQUIRE_EQ(options.buffer_size % options.block_size, 0);

  if (boost::filesystem::is_regular_file(path)) {
    const auto file_size = boost::filesystem::file_size(path);
    MT_ASSERT_EQ(file_size % options.block_size, 0);
    num_blocks_written_ = file_size / options.block_size;

    if (options_.append_only_mode) {
      fd_ = ::open(path.c_str(), O_WRONLY | O_APPEND);
      check(fd_ != -1, "Could not open '%s' in append mode.", path.c_str());

    } else {
      fd_ = ::open(path.c_str(), O_RDWR);
      check(fd_ != -1, "Could not open '%s' in read/write mode.", path.c_str());

      if (file_size != 0) {
        const auto prot = PROT_READ | PROT_WRITE;
        mmap_addr_ = ::mmap64(nullptr, file_size, prot, MAP_SHARED, fd_, 0);
        check(mmap_addr_ != MAP_FAILED,
              "POSIX mmap64() failed for '%s' because: %s", path.c_str(),
              std::strerror(errno));
        num_blocks_mapped_ = num_blocks_written_;
      }
    }
  } else if (options.create_if_missing) {
    const auto permissions = 0644;
    if (options_.append_only_mode) {
      fd_ = ::open(path.c_str(), O_CREAT | O_WRONLY | O_APPEND, permissions);
      check(fd_ != -1, "Could not create '%s' in append mode", path.c_str());

    } else {
      fd_ = ::open(path.c_str(), O_CREAT | O_RDWR, permissions);
      check(fd_ != -1, "Could not create '%s'", path.c_str());
    }
  } else {
    mt::throwRuntimeError2("Does not exist: '%s'", path.c_str());
  }
  buffer_.reset(new char[options.buffer_size]);
  options_ = options;
  path_ = path;
}

std::uint32_t BlockStore::DataFile::append(const Block& block) {
  MT_REQUIRE_NE(fd_, -1);
  MT_REQUIRE_EQ(block.size(), options_.block_size);

  const std::lock_guard<std::mutex> lock(mutex_);
  if (buffer_offset_ == options_.buffer_size) {
    // Flush buffer
    const auto nbytes = ::write(fd_, buffer_.get(), options_.buffer_size);
    check(nbytes != -1, "POSIX write() failed");
    check(static_cast<std::size_t>(nbytes) == options_.buffer_size,
          "POSIX write() wrote only %u of %u bytes", nbytes,
          options_.buffer_size);
    buffer_offset_ = 0;

    if (!options_.append_only_mode) {
      // Remap file
      const auto old_size = options_.block_size * num_blocks_mapped_;
      const auto new_size = options_.block_size * num_blocks_written_;
      if (mmap_addr_) {
        // fsync(fd_);
        // Since Linux provides a so-called unified virtual memory system, it
        // is not necessary to write the content of the buffer cache to disk to
        // ensure that the newly appended data is visible after the remapping.
        // In a unified virtual memory system, memory mappings and blocks of the
        // buffer cache share the same pages of physical memory. [kerrisk p1032]
        mmap_addr_ = ::mremap(mmap_addr_, old_size, new_size, MREMAP_MAYMOVE);
        check(mmap_addr_ != MAP_FAILED, "POSIX mremap() failed");
      } else {
        const auto prot = PROT_READ | PROT_WRITE;
        mmap_addr_ = ::mmap64(nullptr, new_size, prot, MAP_SHARED, fd_, 0);
        check(mmap_addr_ != MAP_FAILED, "POSIX mmap64() failed");
      }
      num_blocks_mapped_ = num_blocks_written_;
    }
  }

  std::memcpy(buffer_.get() + buffer_offset_, block.data(), block.size());
  buffer_offset_ += block.size();

  load_factor_total_ += block.load_factor();
  return num_blocks_written_++;
}

void BlockStore::DataFile::read(std::uint32_t id, Block* block,
                                Arena* arena) const {
  MT_REQUIRE_NE(fd_, -1);
  MT_REQUIRE_NOT_NULL(block);
  check(!options_.append_only_mode,
        "DataFile::read() called in append-only mode");

  if (block->hasData()) {
    MT_ASSERT_EQ(block->size(), options_.block_size);
  } else {
    MT_ASSERT_NOT_NULL(arena);
    block->setData(arena->allocate(options_.block_size), options_.block_size);
  }

  const std::lock_guard<std::mutex> lock(mutex_);
  if (fill_page_cache_) {
    fill_page_cache_ = false;
    // Touch each block to load it into the OS page cache.
    std::vector<char> buffer(options_.block_size);
    for (std::uint32_t i = 0; i != num_blocks_mapped_; ++i) {
      std::memcpy(buffer.data(), getAddressOf(i), options_.block_size);
    }
  }
  std::memcpy(block->data(), getAddressOf(id), block->size());
}

void BlockStore::DataFile::write(std::uint32_t id, const Block& block) {
  MT_REQUIRE_NE(fd_, -1);
  MT_REQUIRE_EQ(block.size(), options_.block_size);
  check(!options_.append_only_mode,
        "DataFile::write() called in append-only mode");

  const std::lock_guard<std::mutex> lock(mutex_);
  std::memcpy(getAddressOf(id), block.data(), block.size());
}

BlockStore::Stats BlockStore::DataFile::getStats() const {
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

const boost::filesystem::path& BlockStore::DataFile::path() const {
  return path_;
}

const BlockStore::Options& BlockStore::DataFile::options() const {
  return options_;
}

char* BlockStore::DataFile::getAddressOf(std::uint32_t id) const {
  MT_REQUIRE_LT(id, num_blocks_written_);
  if (id < num_blocks_mapped_) {
    const auto offset = options_.block_size * id;
    return static_cast<char*>(mmap_addr_) + offset;
  } else {
    const auto offset = options_.block_size * (id - num_blocks_mapped_);
    return buffer_.get() + offset;
  }
}

void BlockStore::DataFile::advise_access_pattern(AccessPattern pattern) const {
  switch (pattern) {
    case AccessPattern::RANDOM:
      fill_page_cache_ = false;
      break;
    case AccessPattern::SEQUENTIAL:
      fill_page_cache_ = true;
      break;
    default:
      mt::throwRuntimeError("Default branch reached");
      break;
  }
}

}  // namespace internal
}  // namespace multimap
