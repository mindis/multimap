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

#ifndef MT_ASSERT_H_
#define MT_ASSERT_H_

#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace mt {

class AssertionError : public std::logic_error {
 public:
  enum class Type { ASSERTION, PRECONDITION, POSTCONDITION };

  enum class Expected { TRUE, FALSE, IS_NULL, IS_ZERO, NOT_NULL, NOT_ZERO };

  explicit AssertionError(const char* message);

  explicit AssertionError(const std::string& message);

  AssertionError(const char* file, size_t line, const char* message);

  AssertionError(const char* file, size_t line, const char* expr,
                 Expected expected, Type type = Type::ASSERTION);

  template <typename Lhs, typename Rhs>
  AssertionError(const char* file, size_t line, const char* expr, Lhs lhs_value,
                 Rhs rhs_value, Type type = Type::ASSERTION);
};

namespace internal {

std::vector<std::string> getStackTrace(size_t skip_frames = 1);

void printStackTraceTo(std::ostream* stream, size_t skip_frames = 2);

void printStackTrace(size_t skip_frames = 3);

template <typename Lhs, typename Rhs>
std::string makeErrorMessage(const char* file, size_t line, const char* expr,
                             Lhs lhs_value, Rhs rhs_value,
                             AssertionError::Type type,
                             size_t skip_head_from_stacktrace) {
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
  printStackTraceTo(&oss, skip_head_from_stacktrace);
  return oss.str();
}

inline void throwError(const char* file, size_t line, const char* message) {
  throw AssertionError(file, line, message);
}

inline void throwError(const char* file, size_t line, const char* expr,
                       AssertionError::Expected expected,
                       AssertionError::Type type) {
  throw AssertionError(file, line, expr, expected, type);
}

template <typename Lhs, typename Rhs>
void throwError(const char* file, size_t line, const char* expr, Lhs lhs_value,
                Rhs rhs_value, AssertionError::Type type) {
  throw AssertionError(file, line, expr, lhs_value, rhs_value, type);
}

template <typename T>
constexpr bool hasExpectedSize(size_t size_on_x32, size_t size_on_x64) {
  return sizeof(T) == ((sizeof(void*) == 4) ? size_on_x32 : size_on_x64);
}
// Checks at compile time if sizeof(T) has some expected value.
//
//   static_assert(mt::internal::hasExpectedSize<long>(4, 8),
//                 "type long does not have expected size");

}  // namespace internal

template <typename Lhs, typename Rhs>
AssertionError::AssertionError(const char* file, size_t line, const char* expr,
                               Lhs lhs_value, Rhs rhs_value, Type type)
    : std::logic_error(internal::makeErrorMessage(file, line, expr, lhs_value,
                                                  rhs_value, type, 4)) {}

#define __MT_VOID ((void)0)

#define __MT_ASSERT_TRUE(expr)                                          \
  (expr) ? __MT_VOID                                                    \
         : mt::internal::throwError(__FILE__, __LINE__, #expr,          \
                                    mt::AssertionError::Expected::TRUE, \
                                    mt::AssertionError::Type::ASSERTION);
#define __MT_ASSERT_FALSE(expr)                                           \
  !(expr) ? __MT_VOID                                                     \
          : mt::internal::throwError(__FILE__, __LINE__, #expr,           \
                                     mt::AssertionError::Expected::FALSE, \
                                     mt::AssertionError::Type::ASSERTION);
#define __MT_ASSERT_NULL(expr)                                              \
  !(expr) ? __MT_VOID                                                       \
          : mt::internal::throwError(__FILE__, __LINE__, #expr,             \
                                     mt::AssertionError::Expected::IS_NULL, \
                                     mt::AssertionError::Type::ASSERTION);
#define __MT_ASSERT_ZERO(expr)                                              \
  !(expr) ? __MT_VOID                                                       \
          : mt::internal::throwError(__FILE__, __LINE__, #expr,             \
                                     mt::AssertionError::Expected::IS_ZERO, \
                                     mt::AssertionError::Type::ASSERTION);
#define __MT_ASSERT_NOT_NULL(expr)                                          \
  (expr) ? __MT_VOID                                                        \
         : mt::internal::throwError(__FILE__, __LINE__, #expr,              \
                                    mt::AssertionError::Expected::NOT_NULL, \
                                    mt::AssertionError::Type::ASSERTION);
#define __MT_ASSERT_NOT_ZERO(expr)                                          \
  (expr) ? __MT_VOID                                                        \
         : mt::internal::throwError(__FILE__, __LINE__, #expr,              \
                                    mt::AssertionError::Expected::NOT_ZERO, \
                                    mt::AssertionError::Type::ASSERTION);
#define __MT_ASSERT_COMPARE(expr, lhs, rhs)                              \
  (expr) ? __MT_VOID                                                     \
         : mt::internal::throwError(__FILE__, __LINE__, #expr, lhs, rhs, \
                                    mt::AssertionError::Type::ASSERTION);

#define __MT_REQUIRE_TRUE(expr)                                         \
  (expr) ? __MT_VOID                                                    \
         : mt::internal::throwError(__FILE__, __LINE__, #expr,          \
                                    mt::AssertionError::Expected::TRUE, \
                                    mt::AssertionError::Type::PRECONDITION);
#define __MT_REQUIRE_FALSE(expr)                                          \
  !(expr) ? __MT_VOID                                                     \
          : mt::internal::throwError(__FILE__, __LINE__, #expr,           \
                                     mt::AssertionError::Expected::FALSE, \
                                     mt::AssertionError::Type::PRECONDITION);
#define __MT_REQUIRE_NULL(expr)                                             \
  !(expr) ? __MT_VOID                                                       \
          : mt::internal::throwError(__FILE__, __LINE__, #expr,             \
                                     mt::AssertionError::Expected::IS_NULL, \
                                     mt::AssertionError::Type::PRECONDITION);
#define __MT_REQUIRE_ZERO(expr)                                             \
  !(expr) ? __MT_VOID                                                       \
          : mt::internal::throwError(__FILE__, __LINE__, #expr,             \
                                     mt::AssertionError::Expected::IS_ZERO, \
                                     mt::AssertionError::Type::PRECONDITION);
#define __MT_REQUIRE_NOT_NULL(expr)                                         \
  (expr) ? __MT_VOID                                                        \
         : mt::internal::throwError(__FILE__, __LINE__, #expr,              \
                                    mt::AssertionError::Expected::NOT_NULL, \
                                    mt::AssertionError::Type::PRECONDITION);
#define __MT_REQUIRE_NOT_ZERO(expr)                                         \
  (expr) ? __MT_VOID                                                        \
         : mt::internal::throwError(__FILE__, __LINE__, #expr,              \
                                    mt::AssertionError::Expected::NOT_ZERO, \
                                    mt::AssertionError::Type::PRECONDITION);
#define __MT_REQUIRE_COMPARE(expr, lhs, rhs)                             \
  (expr) ? __MT_VOID                                                     \
         : mt::internal::throwError(__FILE__, __LINE__, #expr, lhs, rhs, \
                                    mt::AssertionError::Type::PRECONDITION);

#define __MT_ENSURE_TRUE(expr)                                          \
  (expr) ? __MT_VOID                                                    \
         : mt::internal::throwError(__FILE__, __LINE__, #expr,          \
                                    mt::AssertionError::Expected::TRUE, \
                                    mt::AssertionError::Type::POSTCONDITION);
#define __MT_ENSURE_FALSE(expr)                                           \
  !(expr) ? __MT_VOID                                                     \
          : mt::internal::throwError(__FILE__, __LINE__, #expr,           \
                                     mt::AssertionError::Expected::FALSE, \
                                     mt::AssertionError::Type::POSTCONDITION);
#define __MT_ENSURE_NULL(expr)                                              \
  !(expr) ? __MT_VOID                                                       \
          : mt::internal::throwError(__FILE__, __LINE__, #expr,             \
                                     mt::AssertionError::Expected::IS_NULL, \
                                     mt::AssertionError::Type::POSTCONDITION);
#define __MT_ENSURE_ZERO(expr)                                              \
  !(expr) ? __MT_VOID                                                       \
          : mt::internal::throwError(__FILE__, __LINE__, #expr,             \
                                     mt::AssertionError::Expected::IS_ZERO, \
                                     mt::AssertionError::Type::POSTCONDITION);
#define __MT_ENSURE_NOT_NULL(expr)                                          \
  (expr) ? __MT_VOID                                                        \
         : mt::internal::throwError(__FILE__, __LINE__, #expr,              \
                                    mt::AssertionError::Expected::NOT_NULL, \
                                    mt::AssertionError::Type::POSTCONDITION);
#define __MT_ENSURE_NOT_ZERO(expr)                                          \
  (expr) ? __MT_VOID                                                        \
         : mt::internal::throwError(__FILE__, __LINE__, #expr,              \
                                    mt::AssertionError::Expected::NOT_ZERO, \
                                    mt::AssertionError::Type::POSTCONDITION);
#define __MT_ENSURE_COMPARE(expr, lhs, rhs)                              \
  (expr) ? __MT_VOID                                                     \
         : mt::internal::throwError(__FILE__, __LINE__, #expr, lhs, rhs, \
                                    mt::AssertionError::Type::POSTCONDITION);

#define __MT_FAIL(message) \
  mt::internal::throwError(__FILE__, __LINE__, message);

// These assertions/macros are always enabled.

// clang-format off
#define MT_ASSERT_TRUE(expr) __MT_ASSERT_TRUE(expr)
#define MT_ASSERT_FALSE(expr) __MT_ASSERT_FALSE(expr)
#define MT_ASSERT_NULL(expr) __MT_ASSERT_NULL(expr)
#define MT_ASSERT_ZERO(expr) __MT_ASSERT_ZERO(expr)
#define MT_ASSERT_NOT_NULL(expr) __MT_ASSERT_NOT_NULL(expr)
#define MT_ASSERT_NOT_ZERO(expr) __MT_ASSERT_NOT_ZERO(expr)
#define MT_ASSERT_EQ(a, b) __MT_ASSERT_COMPARE(a == b, a, b)
#define MT_ASSERT_NE(a, b) __MT_ASSERT_COMPARE(a != b, a, b)
#define MT_ASSERT_LT(a, b) __MT_ASSERT_COMPARE(a <  b, a, b)
#define MT_ASSERT_LE(a, b) __MT_ASSERT_COMPARE(a <= b, a, b)
#define MT_ASSERT_GT(a, b) __MT_ASSERT_COMPARE(a >  b, a, b)
#define MT_ASSERT_GE(a, b) __MT_ASSERT_COMPARE(a >= b, a, b)

#define MT_REQUIRE_TRUE(expr) __MT_REQUIRE_TRUE(expr)
#define MT_REQUIRE_FALSE(expr) __MT_REQUIRE_FALSE(expr)
#define MT_REQUIRE_NULL(expr) __MT_REQUIRE_NULL(expr)
#define MT_REQUIRE_ZERO(expr) __MT_REQUIRE_ZERO(expr)
#define MT_REQUIRE_NOT_NULL(expr) __MT_REQUIRE_NOT_NULL(expr)
#define MT_REQUIRE_NOT_ZERO(expr) __MT_REQUIRE_NOT_ZERO(expr)
#define MT_REQUIRE_EQ(a, b) __MT_REQUIRE_COMPARE(a == b, a, b)
#define MT_REQUIRE_NE(a, b) __MT_REQUIRE_COMPARE(a != b, a, b)
#define MT_REQUIRE_LT(a, b) __MT_REQUIRE_COMPARE(a <  b, a, b)
#define MT_REQUIRE_LE(a, b) __MT_REQUIRE_COMPARE(a <= b, a, b)
#define MT_REQUIRE_GT(a, b) __MT_REQUIRE_COMPARE(a >  b, a, b)
#define MT_REQUIRE_GE(a, b) __MT_REQUIRE_COMPARE(a >= b, a, b)

#define MT_ENSURE_TRUE(expr) __MT_ENSURE_TRUE(expr)
#define MT_ENSURE_FALSE(expr) __MT_ENSURE_FALSE(expr)
#define MT_ENSURE_NULL(expr) __MT_ENSURE_NULL(expr)
#define MT_ENSURE_ZERO(expr) __MT_ENSURE_ZERO(expr)
#define MT_ENSURE_NOT_NULL(expr) __MT_ENSURE_NOT_NULL(expr)
#define MT_ENSURE_NOT_ZERO(expr) __MT_ENSURE_NOT_ZERO(expr)
#define MT_ENSURE_EQ(a, b) __MT_ENSURE_COMPARE(a == b, a, b)
#define MT_ENSURE_NE(a, b) __MT_ENSURE_COMPARE(a != b, a, b)
#define MT_ENSURE_LT(a, b) __MT_ENSURE_COMPARE(a <  b, a, b)
#define MT_ENSURE_LE(a, b) __MT_ENSURE_COMPARE(a <= b, a, b)
#define MT_ENSURE_GT(a, b) __MT_ENSURE_COMPARE(a >  b, a, b)
#define MT_ENSURE_GE(a, b) __MT_ENSURE_COMPARE(a >= b, a, b)
// clang-format on

#define MT_FAIL(message) __MT_FAIL(message)

#define MT_STATIC_ASSERT_SIZEOF(type, size_on_x32, size_on_x64)                \
  static_assert(mt::internal::hasExpectedSize<type>(size_on_x32, size_on_x64), \
                "type " #type " does not have expected size")
// Shorthand for internal::hasExpectedSize<Type>() that could be placed after a
// type definition in order to get notified when the object's size has changed.
//
//    class SomeClass {
//      long value;
//    };
//
//    MT_STATIC_ASSERT_SIZEOF(SomeClass, 4, 8);

}  // namespace mt

#endif  // MT_ASSERT_H_
