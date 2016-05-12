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

#ifndef MT_COMMON_H_
#define MT_COMMON_H_

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace mt {

// -----------------------------------------------------------------------------
// Types
// -----------------------------------------------------------------------------

typedef unsigned char byte;

typedef std::vector<byte> Bytes;

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------

static const int VERSION = 20160511;

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------

constexpr bool is32BitSystem() { return sizeof(void*) == 4; }

constexpr bool is64BitSystem() { return sizeof(void*) == 8; }

constexpr uint64_t KiB(uint64_t kibibytes) { return kibibytes << 10; }

constexpr uint64_t MiB(uint64_t mebibytes) { return mebibytes << 20; }

constexpr uint64_t GiB(uint64_t gibibytes) { return gibibytes << 30; }

// -----------------------------------------------------------------------------
// Functions / algorithms
// -----------------------------------------------------------------------------

bool isPrime(uint64_t number);

uint64_t nextPrime(uint64_t number);

constexpr bool isPowerOfTwo(uint64_t number) {
  return (number & (number - 1)) == 0;
}

template <typename A, typename B>
auto min(const A& a, const B& b) -> decltype(a < b ? a : b) {
  return a < b ? a : b;
}

template <typename A, typename B>
auto max(const A& a, const B& b) -> decltype(a > b ? a : b) {
  return a > b ? a : b;
}

uint32_t fnv1aHash32(const byte* buf, size_t len);

inline uint32_t fnv1aHash32(const Bytes& bytes) {
  return fnv1aHash32(bytes.data(), bytes.size());
}

uint64_t fnv1aHash64(const byte* buf, size_t len);

inline uint64_t fnv1aHash64(const Bytes& bytes) {
  return fnv1aHash64(bytes.data(), bytes.size());
}

inline size_t fnv1aHash(const byte* buf, size_t len) {
  return is64BitSystem() ? fnv1aHash64(buf, len) : fnv1aHash32(buf, len);
}

inline size_t fnv1aHash(const Bytes& bytes) {
  return fnv1aHash(bytes.data(), bytes.size());
}

// -----------------------------------------------------------------------------
// Functions / logging
// -----------------------------------------------------------------------------

std::string timestamp();

void printTimestamp(std::ostream* stream);

std::ostream& log(std::ostream* stream);

std::ostream& log();

// -----------------------------------------------------------------------------
// Macros
// -----------------------------------------------------------------------------

#define MT_LIKELY(condition) __builtin_expect(condition, true)
#define MT_UNLIKELY(condition) __builtin_expect(condition, false)

#define MT_ENABLE_IF(condition)      \
  template <bool __MT_B = condition, \
            typename std::enable_if<__MT_B>::type* = nullptr>

#define MT_DISABLE_IF(condition)     \
  template <bool __MT_B = condition, \
            typename std::enable_if<!__MT_B>::type* = nullptr>

#define MT_ENABLE_IF_SAME(formal_type, actual_type) \
  template <typename __MT_T = formal_type,            \
            typename std::enable_if<                   \
                std::is_same<__MT_T, actual_type>::value>::type* = nullptr>

// The purpose of MT_ENABLE_IF and MT_ENABLE_IF_SAME could also have been
// implemented via template specialization.  In this case the implementation
// of mutate() would be given only for a concrete template argument in a full
// specialized method definition.  There would be no implementation for the
// default case.
//
//   template <typename Policy> struct Bar {
//
//     void mutate();  // Just a declaration.
//   };
//
//   template <> inline void Bar<Unique>::mutate() { ... }
//
// However, when trying to compile code that calls mutate() on an
// instantiation
// for which no implementation is given (the default case) the linker (!) will
// complain:
//
//   undefined reference to `Bar<Shared>::mutate()'
//
// That means, for the compiler the class Bar<Shared> actually has a mutate()
// method, it's only the implementation which is missing.  Although it
// prevents
// the code to compile, which is what we want, the semantic is a different
// one.
// Instead, we want the class Bar<Shared> to have no mutate() method at all.
//
// The macros enable the desired behavior by making use of the SFINAE
// (Substitution Failure Is Not An Error) technique.  Now the compiler (!)
// will
// complain:
//
//   no matching function for call to `Bar<Shared>::mutate()'

// -----------------------------------------------------------------------------
// 64-bit Portability
// From https://google.github.io/styleguide/cppguide.html#64-bit_Portability
// -----------------------------------------------------------------------------

// printf macros for size_t, in the style of inttypes.h
#ifdef _LP64
#define __PRIS_PREFIX "z"
#else
#define __PRIS_PREFIX
#endif

// Use these macros after a % in a printf format string
// to get correct 32/64 bit behavior, like this:
// size_t size = records.size();
// printf("%" PRIuS "\n", size);

#define PRIdS __PRIS_PREFIX "d"
#define PRIxS __PRIS_PREFIX "x"
#define PRIuS __PRIS_PREFIX "u"
#define PRIXS __PRIS_PREFIX "X"
#define PRIoS __PRIS_PREFIX "o"

}  // namespace mt

#endif  // MT_COMMON_H_
