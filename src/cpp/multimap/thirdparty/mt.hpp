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

#ifndef MT_MT_HPP_INCLUDED
#define MT_MT_HPP_INCLUDED

#include <fstream>
#include <iterator>
#include <map>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#include <boost/filesystem/path.hpp>

namespace mt {

static const std::size_t VERSION = 20151104;

// -----------------------------------------------------------------------------
// COMMON

bool isPrime(std::size_t number);
// Returns `true` if `number` is prime, `false` otherwise.

std::size_t nextPrime(std::size_t number);
// Returns the next prime number that is greater than or equal to `number`.

constexpr bool isPowerOfTwo(std::size_t num) { return (num & (num - 1)) == 0; }

constexpr std::size_t MiB(std::size_t mebibytes) { return mebibytes << 20; }
// Converts a number in mebibytes to the equivalent number in bytes.

constexpr std::size_t GiB(std::size_t gibibytes) { return gibibytes << 30; }
// Converts a number in gibibytes to the equivalent number in bytes.

// -----------------------------------------------------------------------------
// HASHING

std::size_t crc32(const std::string& str);
// Computes and returns the 32-bit CRC checksum for `str`.

std::size_t crc32(const char* data, std::size_t size);
// Computes and returns the 32-bit CRC checksum for `[data, data + size)`.

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

// -----------------------------------------------------------------------------
// ERROR HANDLING

void throwRuntimeError(const char* message);

void throwRuntimeError2(const char* format, ...);

void check(bool expression, const char* format, ...);

std::vector<std::string> getStackTrace(std::size_t skip_head = 1);

void printStackTraceTo(std::ostream& os, std::size_t skip_head = 2);

void printStackTrace(std::size_t skip_head = 3);

struct Messages {
  static constexpr auto COULD_NOT_OPEN = "Could not open '%s' for reading.";
  static constexpr auto COULD_NOT_CREATE = "Could not create '%s' for writing.";
  static constexpr auto NOT_A_REGULAR_FILE = "'%s' is not a regular file.";
  static constexpr auto NOT_A_DIRECTORY = "'%s' is not a directory.";
  static constexpr auto FATAL_ERROR = "Fatal error.";
};

// -----------------------------------------------------------------------------
// INPUT / OUTPUT

template <typename Container>
std::insert_iterator<Container> inserter(Container& container) {
  return std::inserter(container, container.end());
}
// Wrapper for `std::inserter` that only takes a container (and no iterator).

struct Files {
  typedef std::vector<char> Bytes;

  static Bytes readAllBytes(const boost::filesystem::path& filepath);

  static std::vector<std::string> readAllLines(
      const boost::filesystem::path& filepath);

  template <typename Container>
  static void writeLinewise(const Container& container,
                            const boost::filesystem::path& filepath) {
    writeLinewise(container, filepath,
                  [](const typename Container::value_type& value,
                     std::ostream& os) { os << value; });
  }

  template <typename Container, typename PrintTo>
  static void writeLinewise(const Container& container,
                            const boost::filesystem::path& filepath,
                            PrintTo print_to) {
    std::ofstream ofs(filepath.string());
    check(ofs, Messages::COULD_NOT_CREATE, filepath.c_str());

    for (const auto& value : container) {
      print_to(value, ofs);
      ofs << '\n';
    }
  }
};

class AutoCloseFile {
public:
  AutoCloseFile() = default;

  explicit AutoCloseFile(std::FILE* file);

  ~AutoCloseFile();

  AutoCloseFile(AutoCloseFile&& other);
  AutoCloseFile& operator=(AutoCloseFile&& other);

  AutoCloseFile(const AutoCloseFile&) = delete;
  AutoCloseFile& operator=(const AutoCloseFile&) = delete;

  std::FILE* get() const;

  void reset(std::FILE* file = nullptr);

private:
  std::FILE* file_ = nullptr;
};

// -----------------------------------------------------------------------------
// CONFIGURATION

typedef std::map<std::string, std::string> Properties;

Properties readPropertiesFromFile(const std::string& filepath);

void writePropertiesToFile(const Properties& properties,
                           const std::string& filepath);

// -----------------------------------------------------------------------------
// TYPE TRAITS

constexpr bool is32BitSystem() { return sizeof(void*) == 4; }

constexpr bool is64BitSystem() { return sizeof(void*) == 8; }

template <typename T>
constexpr bool hasExpectedSize(std::size_t size_on_32_bit_system,
                               std::size_t size_on_64_bit_system) {
  return sizeof(T) ==
         (is32BitSystem() ? size_on_32_bit_system : size_on_64_bit_system);
}
// Checks at compile time if sizeof(T) has some expected value.
//
//   static_assert(mt::hasExpectedSize<Type>(40, 48)::value,
//                 "class Type does not have expected size");

#define MT_ENABLE_IF(condition)                                                \
  template <bool __MT_B = condition,                                           \
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

#define MT_ENABLE_IF_SAME(generic_type, concrete_type)                         \
  template <typename __MT_T = generic_type,                                    \
            typename std::enable_if<                                           \
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
// However, when trying to compile code that calls mutate() on an instantiation
// for which no implementation is given (the default case) the linker (!) will
// complain:
//
//   undefined reference to `Bar<Shared>::mutate()'
//
// That means, for the compiler the class Bar<Shared> actually has a mutate()
// method, it's only the implementation which is missing.  Although it prevents
// the code to compile, which is what we want, the semantic is a different one.
// Instead, we want the class Bar<Shared> to have no mutate() method at all.
//
// The macros enable the desired behavior by making use of the SFINAE
// (Substitution Failure Is Not An Error) technique.  Now the compiler (!) will
// complain:
//
//   no matching function for call to `Bar<Shared>::mutate()'

// -----------------------------------------------------------------------------
// CONTRACT-BASED PROGRAMMING

class AssertionError : public std::runtime_error {
public:
  enum class Type { ASSERTION, PRECONDITION, POSTCONDITION };

  enum class Expected { TRUE, FALSE, IS_NULL, IS_ZERO, NOT_NULL, NOT_ZERO };

  explicit AssertionError(const char* message);

  explicit AssertionError(const std::string& message);

  AssertionError(const char* file, std::size_t line, const char* expr,
                 Expected expected, Type type = Type::ASSERTION);

  template <typename Lhs, typename Rhs>
  AssertionError(const char* file, std::size_t line, const char* expr,
                 Lhs lhs_value, Rhs rhs_value, Type type = Type::ASSERTION);
};

namespace internal {

template <typename Lhs, typename Rhs>
std::string makeErrorMessage(const char* file, std::size_t line,
                             const char* expr, Lhs lhs_value, Rhs rhs_value,
                             AssertionError::Type type,
                             std::size_t skip_head_from_stacktrace) {
  std::ostringstream oss;
  switch (type) {
    case AssertionError::Type::ASSERTION:
      oss << "Assertion failed";
      break;
    case AssertionError::Type::PRECONDITION:
      oss << "Precondition failed";
      break;
    case AssertionError::Type::POSTCONDITION:
      oss << "Postcondition failed";
      break;
    default:
      throw "The unexpected happened.";
  }
  oss << " in " << file << ':' << line << '\n';
  oss << "The expression '" << expr << "' was false.\n";
  oss << "Value of lhs was: " << lhs_value << '\n';
  oss << "Value of rhs was: " << rhs_value << "\n\n";
  printStackTraceTo(oss, skip_head_from_stacktrace);
  return oss.str();
}

inline void throwError(const char* file, std::size_t line, const char* expr,
                       AssertionError::Expected expected,
                       AssertionError::Type type) {
  throw AssertionError(file, line, expr, expected, type);
}

template <typename Lhs, typename Rhs>
void throwError(const char* file, std::size_t line, const char* expr,
                Lhs lhs_value, Rhs rhs_value, AssertionError::Type type) {
  throw AssertionError(file, line, expr, lhs_value, rhs_value, type);
}

} // namespace internal

template <typename Lhs, typename Rhs>
AssertionError::AssertionError(const char* file, std::size_t line,
                               const char* expr, Lhs lhs_value, Rhs rhs_value,
                               Type type)
    : std::runtime_error(internal::makeErrorMessage(file, line, expr, lhs_value,
                                                    rhs_value, type, 4)) {}

} // namespace mt

#define __MT_VOID ((void)0)

#ifdef NDEBUG

#define __MT_ASSERT_TRUE(expr) __MT_VOID
#define __MT_ASSERT_FALSE(expr) __MT_VOID
#define __MT_ASSERT_IS_NULL(expr) __MT_VOID
#define __MT_ASSERT_IS_ZERO(expr) __MT_VOID
#define __MT_ASSERT_NOT_NULL(expr) __MT_VOID
#define __MT_ASSERT_NOT_ZERO(expr) __MT_VOID
#define __MT_ASSERT_COMPARE(expr, a, b) __MT_VOID

#define __MT_REQUIRE_TRUE(expr) __MT_VOID
#define __MT_REQUIRE_FALSE(expr) __MT_VOID
#define __MT_REQUIRE_IS_NULL(expr) __MT_VOID
#define __MT_REQUIRE_IS_ZERO(expr) __MT_VOID
#define __MT_REQUIRE_NOT_NULL(expr) __MT_VOID
#define __MT_REQUIRE_NOT_ZERO(expr) __MT_VOID
#define __MT_REQUIRE_COMPARE(expr, a, b) __MT_VOID

#define __MT_ENSURE_TRUE(expr) __MT_VOID
#define __MT_ENSURE_FALSE(expr) __MT_VOID
#define __MT_ENSURE_IS_NULL(expr) __MT_VOID
#define __MT_ENSURE_IS_ZERO(expr) __MT_VOID
#define __MT_ENSURE_NOT_NULL(expr) __MT_VOID
#define __MT_ENSURE_NOT_ZERO(expr) __MT_VOID
#define __MT_ENSURE_COMPARE(expr, a, b) __MT_VOID

#else // NDEBUG

#define __MT_ASSERT_TRUE(expr)                                                 \
  (expr) ? __MT_VOID                                                           \
         : mt::internal::throwError(__FILE__, __LINE__, #expr,                 \
                                    mt::AssertionError::Expected::TRUE,        \
                                    mt::AssertionError::Type::ASSERTION);
#define __MT_ASSERT_FALSE(expr)                                                \
  !(expr) ? __MT_VOID                                                          \
          : mt::internal::throwError(__FILE__, __LINE__, #expr,                \
                                     mt::AssertionError::Expected::FALSE,      \
                                     mt::AssertionError::Type::ASSERTION);
#define __MT_ASSERT_IS_NULL(expr)                                              \
  !(expr) ? __MT_VOID                                                          \
          : mt::internal::throwError(__FILE__, __LINE__, #expr,                \
                                     mt::AssertionError::Expected::IS_NULL,    \
                                     mt::AssertionError::Type::ASSERTION);
#define __MT_ASSERT_IS_ZERO(expr)                                              \
  !(expr) ? __MT_VOID                                                          \
          : mt::internal::throwError(__FILE__, __LINE__, #expr,                \
                                     mt::AssertionError::Expected::IS_ZERO,    \
                                     mt::AssertionError::Type::ASSERTION);
#define __MT_ASSERT_NOT_NULL(expr)                                             \
  (expr) ? __MT_VOID                                                           \
         : mt::internal::throwError(__FILE__, __LINE__, #expr,                 \
                                    mt::AssertionError::Expected::NOT_NULL,    \
                                    mt::AssertionError::Type::ASSERTION);
#define __MT_ASSERT_NOT_ZERO(expr)                                             \
  (expr) ? __MT_VOID                                                           \
         : mt::internal::throwError(__FILE__, __LINE__, #expr,                 \
                                    mt::AssertionError::Expected::NOT_ZERO,    \
                                    mt::AssertionError::Type::ASSERTION);
#define __MT_ASSERT_COMPARE(expr, lhs, rhs)                                    \
  (expr) ? __MT_VOID                                                           \
         : mt::internal::throwError(__FILE__, __LINE__, #expr, lhs, rhs,       \
                                    mt::AssertionError::Type::ASSERTION);

#define __MT_REQUIRE_TRUE(expr)                                                \
  (expr) ? __MT_VOID                                                           \
         : mt::internal::throwError(__FILE__, __LINE__, #expr,                 \
                                    mt::AssertionError::Expected::TRUE,        \
                                    mt::AssertionError::Type::PRECONDITION);
#define __MT_REQUIRE_FALSE(expr)                                               \
  !(expr) ? __MT_VOID                                                          \
          : mt::internal::throwError(__FILE__, __LINE__, #expr,                \
                                     mt::AssertionError::Expected::FALSE,      \
                                     mt::AssertionError::Type::PRECONDITION);
#define __MT_REQUIRE_IS_NULL(expr)                                             \
  !(expr) ? __MT_VOID                                                          \
          : mt::internal::throwError(__FILE__, __LINE__, #expr,                \
                                     mt::AssertionError::Expected::IS_NULL,    \
                                     mt::AssertionError::Type::PRECONDITION);
#define __MT_REQUIRE_IS_ZERO(expr)                                             \
  !(expr) ? __MT_VOID                                                          \
          : mt::internal::throwError(__FILE__, __LINE__, #expr,                \
                                     mt::AssertionError::Expected::IS_ZERO,    \
                                     mt::AssertionError::Type::PRECONDITION);
#define __MT_REQUIRE_NOT_NULL(expr)                                            \
  (expr) ? __MT_VOID                                                           \
         : mt::internal::throwError(__FILE__, __LINE__, #expr,                 \
                                    mt::AssertionError::Expected::NOT_NULL,    \
                                    mt::AssertionError::Type::PRECONDITION);
#define __MT_REQUIRE_NOT_ZERO(expr)                                            \
  (expr) ? __MT_VOID                                                           \
         : mt::internal::throwError(__FILE__, __LINE__, #expr,                 \
                                    mt::AssertionError::Expected::NOT_ZERO,    \
                                    mt::AssertionError::Type::PRECONDITION);
#define __MT_REQUIRE_COMPARE(expr, lhs, rhs)                                   \
  (expr) ? __MT_VOID                                                           \
         : mt::internal::throwError(__FILE__, __LINE__, #expr, lhs, rhs,       \
                                    mt::AssertionError::Type::PRECONDITION);

#define __MT_ENSURE_TRUE(expr)                                                 \
  (expr) ? __MT_VOID                                                           \
         : mt::internal::throwError(__FILE__, __LINE__, #expr,                 \
                                    mt::AssertionError::Expected::TRUE,        \
                                    mt::AssertionError::Type::POSTCONDITION);
#define __MT_ENSURE_FALSE(expr)                                                \
  !(expr) ? __MT_VOID                                                          \
          : mt::internal::throwError(__FILE__, __LINE__, #expr,                \
                                     mt::AssertionError::Expected::FALSE,      \
                                     mt::AssertionError::Type::POSTCONDITION);
#define __MT_ENSURE_IS_NULL(expr)                                              \
  !(expr) ? __MT_VOID                                                          \
          : mt::internal::throwError(__FILE__, __LINE__, #expr,                \
                                     mt::AssertionError::Expected::IS_NULL,    \
                                     mt::AssertionError::Type::POSTCONDITION);
#define __MT_ENSURE_IS_ZERO(expr)                                              \
  !(expr) ? __MT_VOID                                                          \
          : mt::internal::throwError(__FILE__, __LINE__, #expr,                \
                                     mt::AssertionError::Expected::IS_ZERO,    \
                                     mt::AssertionError::Type::POSTCONDITION);
#define __MT_ENSURE_NOT_NULL(expr)                                             \
  (expr) ? __MT_VOID                                                           \
         : mt::internal::throwError(__FILE__, __LINE__, #expr,                 \
                                    mt::AssertionError::Expected::NOT_NULL,    \
                                    mt::AssertionError::Type::POSTCONDITION);
#define __MT_ENSURE_NOT_ZERO(expr)                                             \
  (expr) ? __MT_VOID                                                           \
         : mt::internal::throwError(__FILE__, __LINE__, #expr,                 \
                                    mt::AssertionError::Expected::NOT_ZERO,    \
                                    mt::AssertionError::Type::POSTCONDITION);
#define __MT_ENSURE_COMPARE(expr, lhs, rhs)                                    \
  (expr) ? __MT_VOID                                                           \
         : mt::internal::throwError(__FILE__, __LINE__, #expr, lhs, rhs,       \
                                    mt::AssertionError::Type::POSTCONDITION);

#endif // NDEBUG

#define MT_ASSERT_TRUE(expr) __MT_ASSERT_TRUE(expr)
#define MT_ASSERT_FALSE(expr) __MT_ASSERT_FALSE(expr)
#define MT_ASSERT_IS_NULL(expr) __MT_ASSERT_IS_NULL(expr)
#define MT_ASSERT_IS_ZERO(expr) __MT_ASSERT_IS_ZERO(expr)
#define MT_ASSERT_NOT_NULL(expr) __MT_ASSERT_NOT_NULL(expr)
#define MT_ASSERT_NOT_ZERO(expr) __MT_ASSERT_NOT_ZERO(expr)
#define MT_ASSERT_EQ(a, b) __MT_ASSERT_COMPARE(a == b, a, b)
#define MT_ASSERT_NE(a, b) __MT_ASSERT_COMPARE(a != b, a, b)
#define MT_ASSERT_LT(a, b) __MT_ASSERT_COMPARE(a < b, a, b)
#define MT_ASSERT_LE(a, b) __MT_ASSERT_COMPARE(a <= b, a, b)
#define MT_ASSERT_GT(a, b) __MT_ASSERT_COMPARE(a > b, a, b)
#define MT_ASSERT_GE(a, b) __MT_ASSERT_COMPARE(a >= b, a, b)

#define MT_REQUIRE_TRUE(expr) __MT_REQUIRE_TRUE(expr)
#define MT_REQUIRE_FALSE(expr) __MT_REQUIRE_FALSE(expr)
#define MT_REQUIRE_IS_NULL(expr) __MT_REQUIRE_IS_NULL(expr)
#define MT_REQUIRE_IS_ZERO(expr) __MT_REQUIRE_IS_ZERO(expr)
#define MT_REQUIRE_NOT_NULL(expr) __MT_REQUIRE_NOT_NULL(expr)
#define MT_REQUIRE_NOT_ZERO(expr) __MT_REQUIRE_NOT_ZERO(expr)
#define MT_REQUIRE_EQ(a, b) __MT_REQUIRE_COMPARE(a == b, a, b)
#define MT_REQUIRE_NE(a, b) __MT_REQUIRE_COMPARE(a != b, a, b)
#define MT_REQUIRE_LT(a, b) __MT_REQUIRE_COMPARE(a < b, a, b)
#define MT_REQUIRE_LE(a, b) __MT_REQUIRE_COMPARE(a <= b, a, b)
#define MT_REQUIRE_GT(a, b) __MT_REQUIRE_COMPARE(a > b, a, b)
#define MT_REQUIRE_GE(a, b) __MT_REQUIRE_COMPARE(a >= b, a, b)

#define MT_ENSURE_TRUE(expr) __MT_ENSURE_TRUE(expr)
#define MT_ENSURE_FALSE(expr) __MT_ENSURE_FALSE(expr)
#define MT_ENSURE_IS_NULL(expr) __MT_ENSURE_IS_NULL(expr)
#define MT_ENSURE_IS_ZERO(expr) __MT_ENSURE_IS_ZERO(expr)
#define MT_ENSURE_NOT_NULL(expr) __MT_ENSURE_NOT_NULL(expr)
#define MT_ENSURE_NOT_ZERO(expr) __MT_ENSURE_NOT_ZERO(expr)
#define MT_ENSURE_EQ(a, b) __MT_ENSURE_COMPARE(a == b, a, b)
#define MT_ENSURE_NE(a, b) __MT_ENSURE_COMPARE(a != b, a, b)
#define MT_ENSURE_LT(a, b) __MT_ENSURE_COMPARE(a < b, a, b)
#define MT_ENSURE_LE(a, b) __MT_ENSURE_COMPARE(a <= b, a, b)
#define MT_ENSURE_GT(a, b) __MT_ENSURE_COMPARE(a > b, a, b)
#define MT_ENSURE_GE(a, b) __MT_ENSURE_COMPARE(a >= b, a, b)

#endif // MT_MT_HPP_INCLUDED