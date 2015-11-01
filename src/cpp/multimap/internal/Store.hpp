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

#ifndef MULTIMAP_INTERNAL_STORE_HPP_INCLUDED
#define MULTIMAP_INTERNAL_STORE_HPP_INCLUDED

#include <memory>
#include <mutex>
#include <type_traits>
#include <boost/filesystem/path.hpp>
#include "multimap/internal/Arena.hpp"
#include "multimap/internal/Block.hpp"
#include "multimap/thirdparty/mt.hpp"

namespace multimap {
namespace internal {

class Store {
public:
  struct Stats {
    std::uint64_t block_size = 0;
    std::uint64_t num_blocks = 0;
    double load_factor_min = 1.0;
    double load_factor_max = 0;
    double load_factor_avg = 0;

    Stats& combine(const Stats& other);

    static Stats combine(const Stats& a, const Stats& b);

    static Stats fromProperties(const mt::Properties& properties);

    static Stats fromProperties(const mt::Properties& properties,
                                const std::string& prefix);

    mt::Properties toProperties() const;

    mt::Properties toProperties(const std::string& prefix) const;
  };

  static_assert(std::is_standard_layout<Stats>::value,
                "Store::Stats is no standard layout type");

  static_assert(mt::hasExpectedSize<Stats>(40, 40),
                "Store::Stats does not have expected size");
  // Use __attribute__((packed)) if 32- and 64-bit size differ.

  enum class AccessPattern { RANDOM, SEQUENTIAL };

  static const std::size_t DEFAULT_BUFFER_SIZE = mt::MiB(1);

  Store() = default;

  Store(const boost::filesystem::path& filepath, std::size_t block_size,
        std::size_t buffer_size = DEFAULT_BUFFER_SIZE);

  ~Store();

  void open(const boost::filesystem::path& filepath, std::size_t block_size,
            std::size_t buffer_size = DEFAULT_BUFFER_SIZE);

  bool isOpen() const;

  std::uint32_t append(const Block& block);
  // Writes the block to the end of the file and returns its id.
  // Thread-safe: yes.

  void read(std::uint32_t id, Block* block, Arena* arena) const;
  // Thread-safe: yes.

  void write(std::uint32_t id, const Block& block);
  // Thread-safe: yes.

  void adviseAccessPattern(AccessPattern pattern) const;
  // Thread-safe: no.

  Stats getStats() const;
  // Returns various statistics about the store.

  std::size_t block_size() const;
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

    bool empty() const { return offset == 0; }

    std::size_t num_blocks(std::size_t block_size) const {
      return size / block_size;
    }
  };

  mutable std::mutex mutex_;
  mutable bool fill_page_cache_ = false;
  Mapped mapped_;
  Buffer buffer_;
  Stats stats_;
  int fd_ = -1;
};

} // namespace internal
} // namespace multimap

#endif // MULTIMAP_INTERNAL_STORE_HPP_INCLUDED
