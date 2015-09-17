// This file is part of the MT library.  https://bitbucket.org/mtrenkmann/mt
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

#include "mt.hpp"

#include <ios>
#include <cmath>
#include <cstdio>
#include <cstdarg>
#include <vector>

namespace mt {

bool isPrime(std::size_t number) {
  if (number % 2 == 0) {
    return false;
  }
  const std::size_t max = std::sqrt(number);
  for (std::size_t i = 3; i <= max; i += 2) {
    if (number % i == 0) {
      return false;
    }
  }
  return true;
}

std::size_t nextPrime(std::size_t number) {
  while (!isPrime(number)) {
    ++number;
  }
  return number;
}

// Source: http://www.isthe.com/chongo/src/fnv/hash_32a.c
std::uint32_t fnv1aHash32(const void* buf, std::size_t len) {
  std::uint32_t hash = 0x811c9dc5;  // FNV1_32A_INIT
  const auto uchar_buf = reinterpret_cast<const unsigned char*>(buf);
  for (std::size_t i = 0; i != len; ++i) {
    hash ^= uchar_buf[i];
    hash +=
        (hash << 1) + (hash << 4) + (hash << 7) + (hash << 8) + (hash << 24);
  }
  return hash;
}

// Source: http://www.isthe.com/chongo/src/fnv/hash_64a.c
std::uint64_t fnv1aHash64(const void* buf, std::size_t len) {
  std::uint64_t hash = 0xcbf29ce484222325ULL;  // FNV1A_64_INIT
  const auto uchar_buf = reinterpret_cast<const unsigned char*>(buf);
  for (std::size_t i = 0; i != len; ++i) {
    hash ^= uchar_buf[i];
    hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) +
            (hash << 8) + (hash << 40);
  }
  return hash;
}

void throwRuntimeError(const char* message) {
  throw std::runtime_error(message);
}

void throwRuntimeError2(const char* format, ...) {
  va_list args;
  va_start(args, format);
  va_list args2;
  va_copy(args2, args);
  const auto num_bytes = std::vsnprintf(nullptr, 0, format, args) + 1;
  std::vector<char> buffer(num_bytes);
  std::vsnprintf(buffer.data(), buffer.size(), format, args2);
  va_end(args2);
  va_end(args);
  throwRuntimeError(buffer.data());
}

namespace internal {

namespace {

std::string makeErrorMessage(const char* what, const char* file,
                             std::size_t line, const char* expr,
                             bool expected) {
  std::ostringstream oss;
  oss << what << " in " << file << ':' << line;
  oss << "\nThe expression '" << expr << "' should be " << std::boolalpha
      << expected << ", but was " << !expected << '.';
  return oss.str();
}

std::string makeErrorMessage(const char* what, const char* file,
                             std::size_t line, const char* expr) {
  std::ostringstream oss;
  oss << what << " in " << file << ':' << line;
  oss << "\nThe expression '" << expr << "' yields nullptr.";
  return oss.str();
}

}  // namespace

void failAssertTrue(const char* file, std::size_t line, const char* expr) {
  throw std::runtime_error(
      makeErrorMessage("Assertion failed", file, line, expr, true));
}

void failAssertFalse(const char* file, std::size_t line, const char* expr) {
  throw std::runtime_error(
      makeErrorMessage("Assertion failed", file, line, expr, false));
}

void failAssertNotNull(const char* file, std::size_t line, const char* expr) {
  throw std::runtime_error(
      makeErrorMessage("Assertion failed", file, line, expr));
}

void failRequireTrue(const char* file, std::size_t line, const char* expr) {
  throw std::runtime_error(
      makeErrorMessage("Precondition failed", file, line, expr, true));
}

void failRequireFalse(const char* file, std::size_t line, const char* expr) {
  throw std::runtime_error(
      makeErrorMessage("Precondition failed", file, line, expr, false));
}

void failRequireNotNull(const char* file, std::size_t line, const char* expr) {
  throw std::runtime_error(
      makeErrorMessage("Precondition failed", file, line, expr));
}

void failEnsureTrue(const char* file, std::size_t line, const char* expr) {
  throw std::runtime_error(
      makeErrorMessage("Postcondition failed", file, line, expr, true));
}

void failEnsureFalse(const char* file, std::size_t line, const char* expr) {
  throw std::runtime_error(
      makeErrorMessage("Postcondition failed", file, line, expr, false));
}

void failEnsureNotNull(const char* file, std::size_t line, const char* expr) {
  throw std::runtime_error(
      makeErrorMessage("Postcondition failed", file, line, expr));
}

}  // namespace internal
}  // namespace mt
