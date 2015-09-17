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

#ifndef MULTIMAP_INCLUDE_INTERNAL_BLOCK_STORE_HPP
#define MULTIMAP_INCLUDE_INTERNAL_BLOCK_STORE_HPP

#include <map>
#include <mutex>
#include <boost/filesystem/path.hpp>
#include "multimap/internal/Arena.hpp"
#include "multimap/internal/Block.hpp"
#include "multimap/internal/thirdparty/mt.hpp"

namespace multimap {
namespace internal {

enum class AccessPattern { RANDOM, SEQUENTIAL };

class BlockStore {
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
    bool create_if_missing = false;
    std::size_t block_size = 1024;
    std::size_t buffer_size = mt::MiB(1);
    std::size_t num_files = 23;
  };

  BlockStore() = default;

  BlockStore(const boost::filesystem::path& directory, const Options& options);

  void open(const boost::filesystem::path& directory, const Options& options);

  void close();

  std::uint32_t put(const Bytes& key, const Block& block);
  // Inserts a new block into the store and returns a new id.
  // Thread-safe: yes.

  void get(std::uint32_t id, Block* block, Arena* arena) const;
  // Thread-safe: yes.

  void get(std::vector<BlockWithId>* blocks, Arena* arena) const;
  // Thread-safe: yes.

  void replace(std::uint32_t id, const Block& block);
  // Thread-safe: yes.

  void replace(const std::vector<BlockWithId>& blocks);
  // Thread-safe: yes.

  std::uint32_t getFileId(const Bytes& key) const;
  // Thread-safe: yes.

  void adviseAccessPattern(AccessPattern pattern) const;
  // Thread-safe: yes.

  Stats getStats() const;
  // Thread-safe: yes.

  const boost::filesystem::path& directory() const;
  // Thread-safe: yes.

  const Options& options() const;
  // Thread-safe: yes.

  std::size_t num_files() const;
  // Thread-safe: yes.

 private:
  class DataFile {
   public:
    DataFile() = default;

    ~DataFile();

    DataFile(const boost::filesystem::path& path, const Options& options);

    void open(const boost::filesystem::path& path, const Options& options);

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
    Mapped mapped_;
    Buffer buffer_;
    Stats stats_;
    int fd_ = -1;
  };

  mutable std::mutex mutex_;
  std::vector<std::unique_ptr<DataFile>> block_files_;
  boost::filesystem::path directory_;
  Options options_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INCLUDE_INTERNAL_BLOCK_STORE_HPP
