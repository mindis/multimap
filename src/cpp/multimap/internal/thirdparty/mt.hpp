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

#ifndef MT_INCLUDE_MT_HPP
#define MT_INCLUDE_MT_HPP

#include <stdexcept>
#include <sstream>

namespace mt {

static const std::size_t VERSION = 20150917;

// COMMON
// -----------------------------------------------------------------------------

bool isPrime(std::size_t number);
// Returns `true` if `number` is prime, `false` otherwise.

std::size_t nextPrime(std::size_t number);
// Returns the next prime number that is greater than or equal to `number`.

constexpr std::size_t MiB(std::size_t mebibytes) { return mebibytes << 20; }
// Converts a number in mebibytes to the equivalent number in bytes.

constexpr std::size_t GiB(std::size_t gibibytes) { return gibibytes << 30; }
// Converts a number in gibibytes to the equivalent number in bytes.

// HASHING
// -----------------------------------------------------------------------------

std::uint32_t fnv1aHash32(const void* buf, std::size_t len);
// Computes and returns a 32-bit hash value of the given byte array.
// Source: http://www.isthe.com/chongo/src/fnv/fnv.h
// Changes:
//  * Parameter hashval removed, internally set to FNV1_32A_INIT.
//  * More C++ like coding style.

std::uint64_t fnv1aHash64(const void* buf, std::size_t len);
// Computes and returns a 64-bit hash value of the given byte array.
// Source: http://www.isthe.com/chongo/src/fnv/fnv.h
// Changes:
//  * Parameter hashval removed, internally set to FNV1A_64_INIT.
//  * More C++ like coding style.

// ERROR HANDLING
// -----------------------------------------------------------------------------

void throwRuntimeError(const char* message);

void throwRuntimeError2(const char* format, ...);

}  // namespace mt

// CONTRACT-BASED PROGRAMMING
// -----------------------------------------------------------------------------

#define __MT_VOID ((void)0)

#ifdef NDEBUG

#define __MT_ASSERT_TRUE(expr) __MT_VOID
#define __MT_ASSERT_FALSE(expr) __MT_VOID
#define __MT_ASSERT_NOT_NULL(expr) __MT_VOID
#define __MT_ASSERT_COMPARE(expr, a, b) __MT_VOID

#define __MT_REQUIRE_TRUE(expr) __MT_VOID
#define __MT_REQUIRE_FALSE(expr) __MT_VOID
#define __MT_REQUIRE_NOT_NULL(expr) __MT_VOID
#define __MT_REQUIRE_COMPARE(expr, a, b) __MT_VOID

#define __MT_ENSURE_TRUE(expr) __MT_VOID
#define __MT_ENSURE_FALSE(expr) __MT_VOID
#define __MT_ENSURE_NOT_NULL(expr) __MT_VOID
#define __MT_ENSURE_COMPARE(expr, a, b) __MT_VOID

#else  // NDEBUG

namespace mt {
namespace internal {

void failAssertTrue(const char* file, std::size_t line, const char* expr);

void failAssertFalse(const char* file, std::size_t line, const char* expr);

void failAssertNotNull(const char* file, std::size_t line, const char* expr);

void failRequireTrue(const char* file, std::size_t line, const char* expr);

void failRequireFalse(const char* file, std::size_t line, const char* expr);

void failRequireNotNull(const char* file, std::size_t line, const char* expr);

void failEnsureTrue(const char* file, std::size_t line, const char* expr);

void failEnsureFalse(const char* file, std::size_t line, const char* expr);

void failEnsureNotNull(const char* file, std::size_t line, const char* expr);

template <typename A, typename B>
std::string makeErrorMessage(const char* what, const char* file,
                             std::size_t line, const char* expr, A a, B b) {
  std::ostringstream oss;
  oss << what << " in " << file << ':' << line;
  oss << "\nThe expression '" << expr << "' was false.";
  oss << "\nValue of lhs was: " << a;
  oss << "\nValue of rhs was: " << b;
  return oss.str();
}

template <typename A, typename B>
void failAssertCompare(const char* file, std::size_t line, const char* expr,
                       A a, B b) {
  throw std::runtime_error(
      makeErrorMessage("Assertion failed", file, line, expr, a, b));
}

template <typename A, typename B>
void failRequireCompare(const char* file, std::size_t line, const char* expr,
                        A a, B b) {
  throw std::runtime_error(
      makeErrorMessage("Precondition failed", file, line, expr, a, b));
}

template <typename A, typename B>
void failEnsureCompare(const char* file, std::size_t line, const char* expr,
                       A a, B b) {
  throw std::runtime_error(
      makeErrorMessage("Postcondition failed", file, line, expr, a, b));
}

}  // namespace internal
}  // namespace mt

#define __MT_ASSERT_TRUE(expr) \
  (expr) ? __MT_VOID : mt::internal::failAssertTrue(__FILE__, __LINE__, #expr)
#define __MT_ASSERT_FALSE(expr) \
  !(expr) ? __MT_VOID : mt::internal::failAssertFalse(__FILE__, __LINE__, #expr)
#define __MT_ASSERT_NOT_NULL(expr) \
  (expr) ? __MT_VOID               \
         : mt::internal::failAssertNotNull(__FILE__, __LINE__, #expr)
#define __MT_ASSERT_COMPARE(expr, a, b) \
  (expr) ? __MT_VOID                    \
         : mt::internal::failAssertCompare(__FILE__, __LINE__, #expr, a, b)

#define __MT_REQUIRE_TRUE(expr) \
  (expr) ? __MT_VOID : mt::internal::failRequireTrue(__FILE__, __LINE__, #expr)
#define __MT_REQUIRE_FALSE(expr) \
  !(expr) ? __MT_VOID            \
          : mt::internal::failRequireFalse(__FILE__, __LINE__, #expr)
#define __MT_REQUIRE_NOT_NULL(expr) \
  (expr) ? __MT_VOID                \
         : mt::internal::failRequireNotNull(__FILE__, __LINE__, #expr)
#define __MT_REQUIRE_COMPARE(expr, a, b) \
  (expr) ? __MT_VOID                     \
         : mt::internal::failRequireCompare(__FILE__, __LINE__, #expr, a, b)

#define __MT_ENSURE_TRUE(expr) \
  (expr) ? __MT_VOID : mt::internal::failEnsureTrue(__FILE__, __LINE__, #expr)
#define __MT_ENSURE_FALSE(expr) \
  !(expr) ? __MT_VOID : mt::internal::failEnsureFalse(__FILE__, __LINE__, #expr)
#define __MT_ENSURE_NOT_NULL(expr) \
  (expr) ? __MT_VOID               \
         : mt::internal::failEnsureNotNull(__FILE__, __LINE__, #expr)
#define __MT_ENSURE_COMPARE(expr, a, b) \
  (expr) ? __MT_VOID                    \
         : mt::internal::failEnsureCompare(__FILE__, __LINE__, #expr, a, b)

#endif  // NDEBUG

#define MT_ASSERT_TRUE(expr) __MT_ASSERT_TRUE(expr)
#define MT_ASSERT_FALSE(expr) __MT_ASSERT_FALSE(expr)
#define MT_ASSERT_NOT_NULL(expr) __MT_ASSERT_NOT_NULL(expr)
#define MT_ASSERT_EQ(a, b) __MT_ASSERT_COMPARE(a == b, a, b)
#define MT_ASSERT_NE(a, b) __MT_ASSERT_COMPARE(a != b, a, b)
#define MT_ASSERT_LT(a, b) __MT_ASSERT_COMPARE(a <  b, a, b)
#define MT_ASSERT_LE(a, b) __MT_ASSERT_COMPARE(a <= b, a, b)
#define MT_ASSERT_GT(a, b) __MT_ASSERT_COMPARE(a >  b, a, b)
#define MT_ASSERT_GE(a, b) __MT_ASSERT_COMPARE(a >= b, a, b)

#define MT_REQUIRE_TRUE(expr) __MT_REQUIRE_TRUE(expr)
#define MT_REQUIRE_FALSE(expr) __MT_REQUIRE_FALSE(expr)
#define MT_REQUIRE_NOT_NULL(expr) __MT_REQUIRE_NOT_NULL(expr)
#define MT_REQUIRE_EQ(a, b) __MT_REQUIRE_COMPARE(a == b, a, b)
#define MT_REQUIRE_NE(a, b) __MT_REQUIRE_COMPARE(a != b, a, b)
#define MT_REQUIRE_LT(a, b) __MT_REQUIRE_COMPARE(a <  b, a, b)
#define MT_REQUIRE_LE(a, b) __MT_REQUIRE_COMPARE(a <= b, a, b)
#define MT_REQUIRE_GT(a, b) __MT_REQUIRE_COMPARE(a >  b, a, b)
#define MT_REQUIRE_GE(a, b) __MT_REQUIRE_COMPARE(a >= b, a, b)

#define MT_ENSURE_TRUE(expr) __MT_ENSURE_TRUE(expr)
#define MT_ENSURE_FALSE(expr) __MT_ENSURE_FALSE(expr)
#define MT_ENSURE_NOT_NULL(expr) __MT_ENSURE_NOT_NULL(expr)
#define MT_ENSURE_EQ(a, b) __MT_ENSURE_COMPARE(a == b, a, b)
#define MT_ENSURE_NE(a, b) __MT_ENSURE_COMPARE(a != b, a, b)
#define MT_ENSURE_LT(a, b) __MT_ENSURE_COMPARE(a <  b, a, b)
#define MT_ENSURE_LE(a, b) __MT_ENSURE_COMPARE(a <= b, a, b)
#define MT_ENSURE_GT(a, b) __MT_ENSURE_COMPARE(a >  b, a, b)
#define MT_ENSURE_GE(a, b) __MT_ENSURE_COMPARE(a >= b, a, b)

#endif  // MT_INCLUDE_MT_HPP
