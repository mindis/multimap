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

#ifndef MULTIMAP_CALLBACKS_HPP
#define MULTIMAP_CALLBACKS_HPP

#include <functional>
#include <limits>
#include <utility>
#include <vector>
#include "multimap/internal/Block.hpp"

namespace multimap {
namespace internal {

// TODO Detailed documentation.
struct Callbacks {
  static const auto kNoBlockId = std::numeric_limits<std::uint32_t>::max();

  typedef std::function<Block()> AllocateBlock;

  typedef std::function<void(Block&&)> DeallocateBlock;

  typedef std::function<void(std::vector<Block>*)> DeallocateBlocks;

  // Commits a block getting back an id for later identification.
  // Returns kNoBlockId if the given block has no data, i.e. !block.has_data().
  // Note that the block is moved and therefore not usable anymore.
  typedef std::function<std::uint32_t(Block&&)> CommitBlock;

  // Updates the content of an already existing block.
  typedef std::function<void(std::uint32_t, const Block&)> UpdateBlock;

  // Requests the block with the given id which was commited previously.
  // Inits the output parameter with Block::kNullBlock, if the id was unknown.
  typedef std::function<void(std::uint32_t, Block*)> RequestBlock;

  AllocateBlock allocate_block;
  DeallocateBlock deallocate_block;
  DeallocateBlocks deallocate_blocks;
  CommitBlock commit_block;
  UpdateBlock update_block;
  RequestBlock request_block;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_CALLBACKS_HPP
