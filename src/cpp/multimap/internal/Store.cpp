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

#include <cstdio>
#include <boost/filesystem/operations.hpp>

namespace multimap {
namespace internal {

namespace {

const size_t SEGMENT_SIZE = mt::MiB(2);

}  // namespace

Store::Store(const boost::filesystem::path& filename, const Options& options)
    : options_(options) {
  MT_REQUIRE_NOT_ZERO(options.block_size);
  if (boost::filesystem::is_regular_file(filename)) {
    fd_ = mt::open(filename, options.readonly ? O_RDONLY : O_RDWR);
    const uint64_t file_size = mt::seek(fd_.get(), 0, SEEK_END);
    mt::Check::isZero(file_size % options.block_size,
                      "Store: block size does not match size of data file");
    //    if (file_size != 0) {
    //      auto prot = PROT_READ;
    //      if (!options.readonly) {
    //        prot |= PROT_WRITE;
    //      }
    //      mapped_.data =
    //          mt::mmap(file_size, prot, MAP_SHARED, ::fileno(stream_.get()),
    //          0);
    //      mapped_.size = file_size;
    //    }

  } else {
    fd_ = mt::open(filename, O_RDWR | O_CREAT, 0644);
  }
}

Store::~Store() {
  if (fd_ && !options_.readonly) {
    const uint64_t file_size = getNumBlocksUnlocked() * options_.block_size;
    mt::truncate(fd_.get(), file_size);
  }
}

uint32_t Store::put(const Block& block) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (segments_.empty() || segments_.back().isFull()) {
    const uint64_t old_filesize = mt::seek(fd_.get(), 0, SEEK_END);
    const uint64_t new_filesize = old_filesize + SEGMENT_SIZE;
    mt::truncate(fd_.get(), new_filesize);
    segments_.push_back(mt::mmap(SEGMENT_SIZE, PROT_READ | PROT_WRITE,
                                 MAP_SHARED, fd_.get(), old_filesize));
  }
  segments_.back().append(block);
  return getNumBlocksUnlocked() - 1;
}

size_t Store::getNumBlocksUnlocked() const {
  const auto num_segments = segments_.size();
  const auto blocks_per_segment = SEGMENT_SIZE / options_.block_size;
  return (num_segments != 0)
             ? ((num_segments - 1) * blocks_per_segment +
                segments_.back().getNumBlocks(options_.block_size))
             : 0;
}

Store::Segment::Segment(mt::AutoUnmapMemory memory)
    : memory_(std::move(memory)) {}

void Store::Segment::append(const Block& block) {
  MT_REQUIRE_FALSE(isFull());
  std::memcpy(memory_.addr() + offset_, block.data, block.size);
  offset_ += block.size;
}

}  // namespace internal
}  // namespace multimap
