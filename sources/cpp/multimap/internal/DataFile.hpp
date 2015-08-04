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

#ifndef MULTIMAP_INTERNAL_DATA_FILE_HPP
#define MULTIMAP_INTERNAL_DATA_FILE_HPP

#include <functional>
#include <memory>
#include <mutex>
#include <boost/filesystem/path.hpp>
#include "multimap/internal/Block.hpp"
#include "multimap/internal/Callbacks.hpp"

namespace multimap {
namespace internal {

class DataFile {
 public:
  static const std::size_t kMaxBufferSize;

  DataFile();

  DataFile(const boost::filesystem::path& path,
           const Callbacks::DeallocateBlocks& deallocate_blocks);

  DataFile(const boost::filesystem::path& path,
           const Callbacks::DeallocateBlocks& deallocate_blocks,
           bool create_if_missing, std::size_t block_size);

  ~DataFile();

  void Open(const boost::filesystem::path& path,
            const Callbacks::DeallocateBlocks& deallocate_blocks);

  void Open(const boost::filesystem::path& path,
            const Callbacks::DeallocateBlocks& deallocate_blocks,
            bool create_if_missing, std::size_t block_size);

  // Thread-safe: yes.
  void Read(std::uint32_t block_id, Block* block) const;

  // Thread-safe: yes.
  void Write(std::uint32_t block_id, const Block& block);

  // Thread-safe: yes.
  std::uint32_t Append(Block&& block);

  // Thread-safe: yes.
  std::size_t Flush();

  // Thread-safe: yes.
  const boost::filesystem::path& path() const;

  // Thread-safe: yes.
  std::size_t block_size() const;

  // Thread-safe: no.
  std::size_t buffer_size() const;

  // Thread-safe: no.
  SuperBlock super_block() const;

  // Thread-safe: no.
  const Callbacks::DeallocateBlocks& get_deallocate_blocks() const;

  // Thread-safe: no.
  void set_deallocate_blocks(const Callbacks::DeallocateBlocks& callback);

 private:
  std::size_t FlushUnlocked();

  mutable std::mutex mutex_;
  std::vector<Block> buffer_;
  Callbacks::DeallocateBlocks deallocate_blocks_;
  boost::filesystem::path path_;
  SuperBlock super_block_;
  int fd_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_DATA_FILE_HPP
