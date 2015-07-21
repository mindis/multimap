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

#include <unistd.h>
#include "multimap/internal/DataFile.hpp"
#include "multimap/internal/System.hpp"

namespace multimap {
namespace internal {

namespace {

std::uint64_t BlockIdToOffset(std::uint32_t block_id, std::size_t block_size) {
  return internal::SuperBlock::kSerializedSize + block_id * block_size;
}

}  // namespace

const std::size_t DataFile::kMaxBufferSize = sysconf(_SC_IOV_MAX);

DataFile::~DataFile() {
  internal::System::Log() << "DataFile::~DataFile BEGIN\n";
  if (fd_ != -1) {
    FlushUnlocked();
    System::Seek(fd_, 0);
    super_block_.WriteToFd(fd_);
    System::Close(fd_);
    fd_ = -1;
  }
  internal::System::Log() << "DataFile::~DataFile END\n";
}

std::unique_ptr<DataFile> DataFile::Create(const boost::filesystem::path& path,
                                           std::size_t block_size) {
  std::unique_ptr<DataFile> data_file(new DataFile());
  data_file->path_ = path;
  data_file->super_block_.block_size = block_size;
  data_file->fd_ = System::Create(data_file->path_);
  data_file->super_block_.WriteToFd(data_file->fd_);
  return data_file;
}

std::unique_ptr<DataFile> DataFile::Create(
    const boost::filesystem::path& path, std::size_t block_size,
    const Callbacks::DeallocateBlocks& deallocate_blocks) {
  auto data_file = Create(path, block_size);
  data_file->set_deallocate_blocks(deallocate_blocks);
  return data_file;
}

std::unique_ptr<DataFile> DataFile::Open(const boost::filesystem::path& path) {
  std::unique_ptr<DataFile> data_file(new DataFile());
  data_file->path_ = path;
  data_file->fd_ = System::Open(data_file->path_);
  data_file->super_block_ = SuperBlock::ReadFromFd(data_file->fd_);
  // TODO Verify version numbers and throw if not compatible.
  return data_file;
}

std::unique_ptr<DataFile> DataFile::Open(
    const boost::filesystem::path& path,
    const Callbacks::DeallocateBlocks& deallocate_blocks) {
  auto data_file = Open(path);
  data_file->set_deallocate_blocks(deallocate_blocks);
  return data_file;
}

void DataFile::Read(std::uint32_t block_id, Block* block) const {
  assert(block->has_data());
  assert(block->size() == super_block_.block_size);
  const std::lock_guard<std::mutex> lock(mutex_);
  if (block_id < super_block_.num_blocks) {
    const auto offset = BlockIdToOffset(block_id, block->size());
    System::Read(fd_, block->data(), block->size(), offset);
  } else {
    const auto index = block_id - super_block_.num_blocks;
    assert(index < buffer_.size());
    std::memcpy(block->data(), buffer_[index].data(), block->size());
  }
}

void DataFile::Write(std::uint32_t block_id, const Block& block) {
  assert(block.has_data());
  assert(block.size() == super_block_.block_size);
  const std::lock_guard<std::mutex> lock(mutex_);
  if (block_id < super_block_.num_blocks) {
    const auto offset = BlockIdToOffset(block_id, block.size());
    System::Write(fd_, block.data(), block.size(), offset);
  } else {
    const auto index = block_id - super_block_.num_blocks;
    assert(index < buffer_.size());
    std::memcpy(buffer_[index].data(), block.data(), block.size());
  }
}

std::uint32_t DataFile::Append(Block&& block) {
  assert(block.has_data());
  assert(block.size() == super_block_.block_size);
  const std::lock_guard<std::mutex> lock(mutex_);
  if (buffer_.size() == kMaxBufferSize) {
    FlushUnlocked();
  }
  buffer_.push_back(block);
  return super_block_.num_blocks + buffer_.size() - 1;
}

std::size_t DataFile::Flush() {
  const std::lock_guard<std::mutex> lock(mutex_);
  return FlushUnlocked();
}

const boost::filesystem::path& DataFile::path() const { return path_; }

std::size_t DataFile::block_size() const { return super_block_.block_size; }

std::size_t DataFile::buffer_size() const { return buffer_.size(); }

SuperBlock DataFile::super_block() const { return super_block_; }

const Callbacks::DeallocateBlocks& DataFile::get_deallocate_blocks() const {
  return deallocate_blocks_;
}

void DataFile::set_deallocate_blocks(
    const Callbacks::DeallocateBlocks& callback) {
  deallocate_blocks_ = callback;
}

DataFile::DataFile() : fd_(-1) {}

std::size_t DataFile::FlushUnlocked() {
  System::Batch batch;
  for (const auto& block : buffer_) {
    super_block_.load_factor_total += block.load_factor();
    batch.Add(block.data(), block.size());
  }
  batch.Write(fd_);
  super_block_.num_blocks += batch.size();

  if (deallocate_blocks_) {
    deallocate_blocks_(&buffer_);
  }

  buffer_.clear();
  return batch.size();
}

}  // namespace internal
}  // namespace multimap
