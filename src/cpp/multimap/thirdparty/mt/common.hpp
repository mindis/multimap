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

#ifndef MT_COMMON_HPP_INCLUDED
#define MT_COMMON_HPP_INCLUDED

#include <cstdint>
#include <cstring>
#include <string>

namespace mt {

static const int VERSION = 20160418;

bool isPrime(uint64_t number);
// Returns `true` if `number` is prime, `false` otherwise.

uint64_t nextPrime(uint64_t number);
// Returns the next prime number that is greater than or equal to `number`.

constexpr bool isPowerOfTwo(uint64_t num) { return (num & (num - 1)) == 0; }

constexpr uint64_t KiB(uint64_t kibibytes) { return kibibytes << 10; }
// Converts a number in kibibytes to the equivalent number in bytes.

constexpr uint64_t MiB(uint64_t mebibytes) { return mebibytes << 20; }
// Converts a number in mebibytes to the equivalent number in bytes.

constexpr uint64_t GiB(uint64_t gibibytes) { return gibibytes << 30; }
// Converts a number in gibibytes to the equivalent number in bytes.

constexpr bool is32BitSystem() { return sizeof(void*) == 4; }

constexpr bool is64BitSystem() { return sizeof(void*) == 8; }

uint64_t currentResidentMemory();

// -----------------------------------------------------------------------------
// ALGORITHM
// -----------------------------------------------------------------------------

uint32_t fnv1aHash32(const void* buf, size_t len);
// Computes and returns a 32-bit hash value of the given byte array.
// Source: http://www.isthe.com/chongo/src/fnv/fnv.h
// Changes:
//  * Parameter hashval removed, internally set to FNV1_32A_INIT.
//  * More C++ like coding style.

uint64_t fnv1aHash64(const void* buf, size_t len);
// Computes and returns a 64-bit hash value of the given byte array.
// Source: http://www.isthe.com/chongo/src/fnv/fnv.h
// Changes:
//  * Parameter hashval removed, internally set to FNV1A_64_INIT.
//  * More C++ like coding style.

inline size_t fnv1aHash(const void* buf, size_t len) {
  return is64BitSystem() ? fnv1aHash64(buf, len) : fnv1aHash32(buf, len);
  // The compiler will optimize away branching here.
}
// Dispatching function to choose either `fnv1aHash32()` or `fnv1aHash64()`
// depending on the actual system.

template <typename A, typename B>
auto min(const A& a, const B& b) -> decltype(a < b ? a : b) {
  return a < b ? a : b;
}
// `std::min` requires that `a` and `b` are of the same type which means
// that you cannot compare `std::int32_t` and `std::int64_t` without casting.

template <typename A, typename B>
auto max(const A& a, const B& b) -> decltype(a > b ? a : b) {
  return a > b ? a : b;
}
// `std::min` requires that `a` and `b` are of the same type which means
// that you cannot compare `std::int32_t` and `std::int64_t` without casting.

// -----------------------------------------------------------------------------
// LOGGING
// -----------------------------------------------------------------------------

std::string timestamp();

void printTimestamp(std::ostream& stream);

std::ostream& log(std::ostream& stream);
// Usage: mt::log(some_stream) << "Some message\n";

std::ostream& log();
// Usage: mt::log() << "Some message\n";

// -----------------------------------------------------------------------------
// TYPE TRAITS
// -----------------------------------------------------------------------------

#define MT_ENABLE_IF(condition)      \
  template <bool __MT_B = condition, \
            typename std::enable_if<__MT_B>::type* = nullptr>
// Enables a method of a class template if a boolean parameter is true.
//
//   template <bool IsMutable> struct Foo {
//
//     MT_ENABLE_IF(IsMutable) void mutate() { ... }
//
//     // Foo<true>().mutate();
//     // Compiles fine.
//     //
//     // Foo<false>().mutate();
//     // Error: no matching function for call to `Foo<false>::mutate()'.
//   };

#define MT_DISABLE_IF(condition)     \
  template <bool __MT_B = condition, \
            typename std::enable_if<!__MT_B>::type* = nullptr>
// Disables a method of a class template if a boolean parameter is true.

#define MT_ENABLE_IF_SAME(generic_type, concrete_type) \
  template <typename __MT_T = generic_type,            \
            typename std::enable_if<                   \
                std::is_same<__MT_T, concrete_type>::value>::type* = nullptr>
// Enables a method of a class template for some concrete type argument.
//
//   struct Unique {};
//   struct Shared {};
//
//   template <typename Policy> struct Bar {
//
//     MT_ENABLE_IF_SAME(Policy, Unique) void mutate() { ... }
//
//     // Bar<Unique>().mutate();
//     // Compiles fine.
//
//     // Bar<Shared>().mutate();
//     // Error: no matching function for call to `Bar<Shared>::mutate()'.
//   };

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

#endif  // MT_MT_HPP_INCLUDED
