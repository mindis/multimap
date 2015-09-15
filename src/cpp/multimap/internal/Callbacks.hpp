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

#ifndef MULTIMAP_INCLUDE_INTERNAL_CALLBACKS_HPP
#define MULTIMAP_INCLUDE_INTERNAL_CALLBACKS_HPP

#include <functional>
#include <vector>
#include "multimap/internal/Arena.hpp"
#include "multimap/internal/Block.hpp"

namespace multimap {
namespace internal {

struct Callbacks {
  typedef std::function<Block()> NewBlock;

  typedef std::function<std::uint32_t(const Block&)> CommitBlock;
  // Commits a block getting back an id for later identification.

  typedef std::function<void(const std::vector<BlockWithId>&)> ReplaceBlocks;

  typedef std::function<void(std::vector<BlockWithId>*, Arena*)> RequestBlocks;

  NewBlock new_block;
  CommitBlock commit_block;
  ReplaceBlocks replace_blocks;
  RequestBlocks request_blocks;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INCLUDE_INTERNAL_CALLBACKS_HPP
