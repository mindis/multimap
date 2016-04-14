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

#include "mt/memory.hpp"

#include <unistd.h>
#include "mt/check.hpp"

namespace mt {

AutoUnmapMemory::AutoUnmapMemory(Memory memory) : memory_(memory) {}

AutoUnmapMemory::AutoUnmapMemory(uint8_t* data, size_t size)
    : memory_(data, size) {}

AutoUnmapMemory::AutoUnmapMemory(AutoUnmapMemory&& other)
    : memory_(other.release()) {}

AutoUnmapMemory::~AutoUnmapMemory() { reset(); }

AutoUnmapMemory& AutoUnmapMemory::operator=(AutoUnmapMemory&& other) {
  reset(other.release());
  return *this;
}

AutoUnmapMemory::Memory AutoUnmapMemory::release() {
  const Memory memory = memory_;
  memory_ = Memory();
  return memory;
}

void AutoUnmapMemory::reset(Memory memory) {
  if (memory_.first != nullptr) {
    const int result = ::munmap(memory_.first, memory_.second);
    Check::isZero(result, "munmap() failed because of '%s'", errnostr());
  }
  memory_ = memory;
}

AutoUnmapMemory::operator bool() const noexcept { return data() != nullptr; }

AutoUnmapMemory mmap(size_t length, int prot, int flags, int fd,
                     size_t offset) {
  void* ptr = ::mmap(nullptr, length, prot, flags, fd, offset);
  Check::notEqual(MAP_FAILED, ptr, "mmap() failed because of '%s'", errnostr());
  return AutoUnmapMemory(static_cast<uint8_t*>(ptr), length);
}

uint8_t* getPageBegin(const uint8_t* ptr) {
  size_t value_of_ptr = reinterpret_cast<size_t>(ptr);
  value_of_ptr -= value_of_ptr % getPageSize();
  return reinterpret_cast<uint8_t*>(value_of_ptr);
}

size_t getPageSize() { return sysconf(_SC_PAGESIZE); }

}  // namespace mt
