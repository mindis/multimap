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

#include <map>
#include <mutex>
#include <boost/filesystem/path.hpp>
#include "multimap/internal/Arena.hpp"
#include "multimap/internal/Block.hpp"

namespace multimap {
namespace internal {

class DataFile {
 public:
  struct Stats {
    std::size_t block_size = 0;
    std::size_t num_blocks = 0;
    double load_factor_min = 0;
    double load_factor_max = 0;
    double load_factor_avg = 0;

    std::map<std::string, std::string> ToMap() const;

    std::map<std::string, std::string> ToMap(const std::string& prefix) const;
  };

  struct Options {
    bool create_if_missing = false;
    std::size_t block_size = 1024;
    std::size_t remap_every_nblocks = 1024;
  };

  DataFile();

  DataFile(const boost::filesystem::path& path, const Options& options);

  ~DataFile();

  void Open(const boost::filesystem::path& path, const Options& options);

  void Close();

  // Thread-safe: yes.
  std::uint32_t Append(const Block& block);

  // Thread-safe: yes.
  void Read(std::uint32_t block_id, Block* block, Arena* arena) const;

  // Thread-safe: yes.
  void Read(std::vector<BlockWithId>* blocks, Arena* arena) const;

  // Thread-safe: yes.
  void Write(std::uint32_t block_id, const Block& block);

  // Thread-safe: yes.
  void Write(const std::vector<BlockWithId>& blocks);

  // Thread-safe: yes.
  Stats GetStats() const;

  // Thread-safe: yes.
  const boost::filesystem::path& path() const;

  // Thread-safe: yes.
  const Options& options() const;

 private:
  std::uint64_t offset(std::uint32_t block_id) const;
  char* address(std::uint32_t block_id) const;

  mutable std::mutex mutex_;
  std::size_t num_blocks_written_;
  std::size_t num_blocks_mapped_;
  double load_factor_total_;
  void* data_;
  int fd_;

  boost::filesystem::path path_;
  Options options_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_DATA_FILE_HPP
