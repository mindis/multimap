// This file is part of the MT library.
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

#ifndef MT_MEMORY_HPP_INCLUDED
#define MT_MEMORY_HPP_INCLUDED

#include <sys/mman.h>  // PROT_READ, PROT_WRITE, etc.
#include <cstddef>
#include <cstdint>
#include <utility>

namespace mt {

class AutoUnmapMemory {
  // A RAII-style memory mapping owner.
  // The owned memory mapping, if any, is unmapped on destruction.

 public:
  typedef std::pair<uint8_t*, size_t> Memory;

  AutoUnmapMemory() = default;

  AutoUnmapMemory(Memory memory);

  AutoUnmapMemory(uint8_t* data, size_t size);

  AutoUnmapMemory(AutoUnmapMemory&& other);

  ~AutoUnmapMemory();

  AutoUnmapMemory& operator=(AutoUnmapMemory&& other);

  const Memory& get() const { return memory_; }

  uint8_t* data() const { return memory_.first; }

  size_t size() const { return memory_.second; }

  uint8_t* begin() const { return memory_.first; }

  uint8_t* end() const { return memory_.first + memory_.second; }

  Memory release();

  void reset(Memory memory = Memory());

  explicit operator bool() const noexcept;

 private:
  Memory memory_;
};

AutoUnmapMemory mmap(size_t length, int prot, int flags, int fd, size_t offset);
// Tries to map a file into memory or throws std::runtime_error on failure.
// Wraps mmap() from http://man7.org/linux/man-pages/man2/mmap.2.html
// Note that the addr parameter is omited. It is set to nullptr internally.

uint8_t* getPageBegin(const uint8_t* ptr);
// Returns the address of the page where ptr belongs to.

size_t getPageSize();
// Returns the systems page size.

}  // namespace mt

#endif  // MT_MEMORY_HPP_INCLUDED
