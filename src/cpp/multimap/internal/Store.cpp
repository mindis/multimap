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

#include "multimap/internal/Store.h"

#include <cstdio>
#include <boost/filesystem/operations.hpp>  // NOLINT
#include "multimap/thirdparty/mt/assert.h"
#include "multimap/thirdparty/mt/check.h"
#include "multimap/thirdparty/mt/memory.h"

namespace multimap {
namespace internal {

namespace {

const size_t SEGMENT_SIZE = mt::MiB(2);

}  // namespace

Store::Store() : mutex_(new std::mutex()) {}

Store::Store(const boost::filesystem::path& file_path, const Options& options)
    : mutex_(new std::mutex()), options_(options) {
  MT_REQUIRE_NOT_ZERO(options.block_size);
  if (boost::filesystem::is_regular_file(file_path)) {
    fd_ = mt::open(file_path, options.readonly ? O_RDONLY : O_RDWR);
    const uint64_t file_size = mt::lseek(fd_.get(), 0, SEEK_END);
    mt::Check::isZero(file_size % options.block_size,
                      "Store: block size does not match size of data file");
    const size_t num_full_segments = file_size / SEGMENT_SIZE;
    const auto prot = options.readonly ? PROT_READ : (PROT_READ | PROT_WRITE);
    for (size_t i = 0; i != num_full_segments; i++) {
      segments_.emplace_back(
          mt::mmap(SEGMENT_SIZE, prot, MAP_SHARED, fd_.get(), SEGMENT_SIZE * i),
          SEGMENT_SIZE);
    }
    const uint64_t offset = SEGMENT_SIZE * num_full_segments;
    const size_t last_segment_size = file_size % SEGMENT_SIZE;
    if (last_segment_size != 0) {
      if (options.readonly) {
        segments_.emplace_back(
            mt::mmap(last_segment_size, prot, MAP_SHARED, fd_.get(), offset),
            last_segment_size);
      } else {
        mt::ftruncate(fd_.get(), SEGMENT_SIZE * (num_full_segments + 1));
        segments_.emplace_back(
            mt::mmap(SEGMENT_SIZE, prot, MAP_SHARED, fd_.get(), offset),
            last_segment_size);
      }
    }
  } else {
    fd_ = mt::open(file_path, O_RDWR | O_CREAT, 0644);
  }
}

Store::~Store() {
  if (fd_ && !options_.readonly) {
    const uint64_t file_size = getNumBlocksUnlocked() * options_.block_size;
    mt::ftruncate(fd_.get(), file_size);
  }
}

uint32_t Store::put(const Block& block) {
  std::lock_guard<std::mutex> lock(*mutex_);
  if (segments_.empty() || segments_.back().isFull()) {
    const uint64_t old_file_size = segments_.size() * SEGMENT_SIZE;
    const uint64_t new_file_size = old_file_size + SEGMENT_SIZE;
    mt::ftruncate(fd_.get(), new_file_size);
    segments_.emplace_back(mt::mmap(SEGMENT_SIZE, PROT_READ | PROT_WRITE,
                                    MAP_SHARED, fd_.get(), old_file_size));
  }
  segments_.back().append(block);
  return getNumBlocksUnlocked() - 1;
}

Store::Blocks Store::get(const BlockIds& block_ids) const {
  Blocks blocks;
  blocks.reserve(block_ids.size());
  const size_t blocks_per_segment = SEGMENT_SIZE / options_.block_size;
  std::lock_guard<std::mutex> lock(*mutex_);
  for (uint32_t block_id : block_ids) {
    const size_t seg_id = block_id / blocks_per_segment;
    const size_t seg_block_id = block_id % blocks_per_segment;
    MT_ASSERT_LT(seg_id, segments_.size());
    blocks.push_back(segments_[seg_id].get(seg_block_id, options_.block_size));
  }
  return blocks;
}

size_t Store::getNumBlocksUnlocked() const {
  const size_t num_segments = segments_.size();
  const size_t blocks_per_segment = SEGMENT_SIZE / options_.block_size;
  return (num_segments != 0)
             ? ((num_segments - 1) * blocks_per_segment +
                segments_.back().getNumBlocks(options_.block_size))
             : 0;
}

Store::Segment::Segment(mt::AutoUnmapMemory memory)
    : memory_(std::move(memory)) {}

Store::Segment::Segment(mt::AutoUnmapMemory memory, size_t offset)
    : memory_(std::move(memory)), offset_(offset) {
  MT_REQUIRE_LE(offset_, memory_.size())
}

void Store::Segment::append(const Block& block) {
  MT_REQUIRE_FALSE(isFull());
  std::memcpy(memory_.data() + offset_, block.data, block.size);
  offset_ += block.size;
}

Store::Block Store::Segment::get(uint32_t block_id, size_t block_size) const {
  MT_REQUIRE_LT(block_id, getNumBlocks(block_size));
  Block block;
  block.data = memory_.data() + block_id * block_size;
  block.size = block_size;
  return block;
}

}  // namespace internal
}  // namespace multimap
