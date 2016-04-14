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

#include "mt/common.hpp"

#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "mt/check.hpp"

namespace mt {

bool isPrime(uint64_t number) {
  if (number % 2 == 0) {
    return false;
  }
  const uint64_t max = std::sqrt(number);
  for (uint64_t i = 3; i <= max; i += 2) {
    if (number % i == 0) {
      return false;
    }
  }
  return true;
}

uint64_t nextPrime(uint64_t number) {
  while (!isPrime(number)) {
    ++number;
  }
  return number;
}

uint64_t currentResidentMemory() {
  const char* property = "VmRSS:";
  const char* filename = "/proc/self/status";
  std::ifstream input(filename);
  check(input.is_open(), "Could not open %s", filename);
  std::string token;
  uint64_t mem_in_kb;
  while (input >> token) {
    if (token == property) {
      input >> mem_in_kb;
      return KiB(mem_in_kb);
    }
  }
  fail("Could not find property %s in %s", property, filename);
  return 0;
}

// Source: http://www.isthe.com/chongo/src/fnv/hash_32a.c
uint32_t fnv1aHash32(const void* data, size_t size) {
  uint32_t h = 0x811c9dc5;  // FNV1_32A_INIT
  const auto ptr = reinterpret_cast<const std::uint8_t*>(data);
  for (size_t i = 0; i != size; ++i) {
    h ^= ptr[i];
    h += (h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24);
  }
  return h;
}

// Source: http://www.isthe.com/chongo/src/fnv/hash_64a.c
uint64_t fnv1aHash64(const void* data, size_t size) {
  uint64_t h = 0xcbf29ce484222325ULL;  // FNV1A_64_INIT
  const auto ptr = reinterpret_cast<const std::uint8_t*>(data);
  for (size_t i = 0; i != size; ++i) {
    h ^= ptr[i];
    h += (h << 1) + (h << 4) + (h << 5) + (h << 7) + (h << 8) + (h << 40);
  }
  return h;
}

std::string timestamp() {
  std::ostringstream stream;
  printTimestamp(stream);
  return stream.str();
}

void printTimestamp(std::ostream& stream) {
  const std::time_t seconds = std::time(nullptr);
  Check::notEqual(-1, seconds, "std::time() failed because of %s", errnostr());
  struct tm broken_down_time;
  ::localtime_r(&seconds, &broken_down_time);

  char old_fill_char = stream.fill('0');
  stream << broken_down_time.tm_year + 1900;
  stream << '-' << std::setw(2) << broken_down_time.tm_mon + 1;
  stream << '-' << std::setw(2) << broken_down_time.tm_mday;
  stream << ' ' << std::setw(2) << broken_down_time.tm_hour;
  stream << ':' << std::setw(2) << broken_down_time.tm_min;
  stream << ':' << std::setw(2) << broken_down_time.tm_sec;
  stream.fill(old_fill_char);
}

std::ostream& log(std::ostream& stream) {
  printTimestamp(stream);
  return stream.put(' ');
}

std::ostream& log() { return log(std::clog); }

}  // namespace mt
