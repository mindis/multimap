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
#include <memory>
#include <mutex>
#include <boost/filesystem/path.hpp>
#include "multimap/internal/thirdparty/mt.hpp"
#include "multimap/internal/Arena.hpp"
#include "multimap/internal/Block.hpp"

namespace multimap {
namespace internal {

class Store {
 public:
  struct Stats {
    std::size_t block_size = 0;
    std::size_t num_blocks = 0;
    double load_factor_min = 1.0;
    double load_factor_max = 0;
    double load_factor_avg = 0;

    void combine(const Stats& other);

    std::map<std::string, std::string> toMap() const;

    std::map<std::string, std::string> toMap(const std::string& prefix) const;
  };

  enum class AccessPattern { RANDOM, SEQUENTIAL };

  static const std::size_t DEFAULT_BUFFER_SIZE = mt::MiB(1);

  Store() = default;

  ~Store();

  Store(const boost::filesystem::path& prefix,
        std::size_t buffer_size = DEFAULT_BUFFER_SIZE);

  Store(const boost::filesystem::path& prefix, std::size_t block_size,
        std::size_t buffer_size = DEFAULT_BUFFER_SIZE);

  void open(const boost::filesystem::path& prefix,
            std::size_t buffer_size = DEFAULT_BUFFER_SIZE);

  void create(const boost::filesystem::path& prefix, std::size_t block_size,
              std::size_t buffer_size = DEFAULT_BUFFER_SIZE);

  std::uint32_t append(const Block& block);
  // Writes the block to the end of the file and returns its id.
  // Thread-safe: yes.

  void read(std::uint32_t id, Block* block, Arena* arena) const;
  // Thread-safe: yes.

  void write(std::uint32_t id, const Block& block);
  // Thread-safe: yes.

  void adviseAccessPattern(AccessPattern pattern) const;
  // Thread-safe: no.

  std::size_t block_size() const;

  Stats getStats() const;
  // Thread-safe: yes.

 private:
  char* getAddressOf(std::uint32_t id) const;
  // Thread-safe: no.

  struct Mapped {
    void* data = nullptr;
    std::size_t size = 0;

    std::size_t num_blocks(std::size_t block_size) const {
      return size / block_size;
    }
  };

  struct Buffer {
    std::unique_ptr<char[]> data;
    std::size_t offset = 0;
    std::size_t size = 0;

    std::size_t num_blocks(std::size_t block_size) const {
      return size / block_size;
    }
  };

  mutable std::mutex mutex_;
  mutable bool fill_page_cache_ = false;
  boost::filesystem::path prefix_;
  Mapped mapped_;
  Buffer buffer_;
  Stats stats_;
  int fd_ = -1;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INCLUDE_INTERNAL_STORE_HPP
