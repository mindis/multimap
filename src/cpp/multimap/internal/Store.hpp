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

#ifndef MULTIMAP_INCLUDE_INTERNAL_STORE_HPP
#define MULTIMAP_INCLUDE_INTERNAL_STORE_HPP

#include <map>
#include <mutex>
#include <boost/filesystem/path.hpp>
#include "multimap/internal/Arena.hpp"
#include "multimap/internal/Block.hpp"
#include "multimap/common.hpp"

namespace multimap {
namespace internal {

class Store {
 public:
  struct Stats {
    std::size_t block_size = 0;
    std::size_t num_blocks = 0;
    double load_factor_min = 0;
    double load_factor_max = 0;
    double load_factor_avg = 0;

    std::map<std::string, std::string> toMap() const;

    std::map<std::string, std::string> toMap(const std::string& prefix) const;
  };

  struct Options {
    bool append_only_mode = false;
    bool create_if_missing = false;
    std::size_t block_size = 1024;
    std::size_t buffer_size = MiB(10);
  };

  Store() = default;

  Store(const boost::filesystem::path& path, const Options& options);

  ~Store();

  void open(const boost::filesystem::path& path, const Options& options);

  void close();

  // Thread-safe: yes.
  std::uint32_t append(const Block& block);

  // Thread-safe: yes.
  void read(std::uint32_t block_id, Block* block, Arena* arena) const;

  // Thread-safe: yes.
  void read(std::vector<BlockWithId>* blocks, Arena* arena) const;

  // Thread-safe: yes.
  void write(std::uint32_t block_id, const Block& block);

  // Thread-safe: yes.
  void write(const std::vector<BlockWithId>& blocks);

  // Thread-safe: yes.
  Stats getStats() const;

  // Thread-safe: yes.
  const boost::filesystem::path& path() const;

  // Thread-safe: yes.
  const Options& options() const;

 private:
  // Thread-safe: no.
  char* getAddressOf(std::uint32_t block_id) const;

  mutable std::mutex mutex_;
  std::unique_ptr<char[]> buffer_;
  std::size_t num_blocks_written_ = 0;
  std::size_t num_blocks_mapped_ = 0;
  std::size_t buffer_offset_ = 0;
  double load_factor_total_ = 0;
  void* mmap_addr_ = nullptr;
  int fd_ = -1;

  boost::filesystem::path path_;
  Options options_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INCLUDE_INTERNAL_STORE_HPP
