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

#ifndef MT_CHECK_H_
#define MT_CHECK_H_

#include <cstdarg>
#include <stdexcept>
#include <string>

namespace mt {
namespace internal {

std::string toString(const char* format, va_list args);

void toString(const char* format, va_list args, std::string* output);

}  // namespace internal

#define __MT_TO_STRING(format, args, output) \
  va_start(args, format);                    \
  internal::toString(format, args, output);  \
  va_end(args)

template <typename Error = std::runtime_error>
void fail(const char* format, ...) {
  va_list args;
  std::string message;
  __MT_TO_STRING(format, args, &message);
  throw Error(message);
}

template <typename Error = std::runtime_error>
void check(bool expression, const char* format, ...) {
  if (!expression) {
    va_list args;
    std::string message;
    __MT_TO_STRING(format, args, &message);
    throw Error(message);
  }
}

const char* errnostr();
// Returns a textual description of the current errno value.
// Wraps strerror() from http://man7.org/linux/man-pages/man3/strerror.3.html

struct Check {
  static void isTrue(bool expression, const char* format, ...) {
    if (!expression) {
      va_list args;
      std::string message;
      __MT_TO_STRING(format, args, &message);
      throw std::runtime_error(message);
    }
  }

  static void isFalse(bool expression, const char* format, ...) {
    if (expression) {
      va_list args;
      std::string message;
      __MT_TO_STRING(format, args, &message);
      throw std::runtime_error(message);
    }
  }

  template <typename T>
  static void isNull(const T* pointer, const char* format, ...) {
    if (pointer != nullptr) {
      va_list args;
      std::string message;
      __MT_TO_STRING(format, args, &message);
      throw std::runtime_error(message);
    }
  }

  template <typename T>
  static void notNull(const T* pointer, const char* format, ...) {
    if (pointer == nullptr) {
      va_list args;
      std::string message;
      __MT_TO_STRING(format, args, &message);
      throw std::runtime_error(message);
    }
  }

  template <typename T>
  static void isZero(const T& value, const char* format, ...) {
    if (value != 0) {
      va_list args;
      std::string message;
      __MT_TO_STRING(format, args, &message);
      throw std::runtime_error(message);
    }
  }

  template <typename T>
  static void notZero(const T& value, const char* format, ...) {
    if (value == 0) {
      va_list args;
      std::string message;
      __MT_TO_STRING(format, args, &message);
      throw std::runtime_error(message);
    }
  }

  template <typename A, typename B>
  static void isEqual(const A& a, const B& b, const char* format, ...) {
    if (!(a == b)) {
      va_list args;
      std::string message;
      __MT_TO_STRING(format, args, &message);
      throw std::runtime_error(message);
    }
  }

  template <typename A, typename B>
  static void notEqual(const A& a, const B& b, const char* format, ...) {
    if (!(a != b)) {
      va_list args;
      std::string message;
      __MT_TO_STRING(format, args, &message);
      throw std::runtime_error(message);
    }
  }

  template <typename A, typename B>
  static void isLessThan(const A& a, const B& b, const char* format, ...) {
    if (!(a < b)) {
      va_list args;
      std::string message;
      __MT_TO_STRING(format, args, &message);
      throw std::runtime_error(message);
    }
  }

  template <typename A, typename B>
  static void isLessEqual(const A& a, const B& b, const char* format, ...) {
    if (!(a <= b)) {
      va_list args;
      std::string message;
      __MT_TO_STRING(format, args, &message);
      throw std::runtime_error(message);
    }
  }

  template <typename A, typename B>
  static void isGreaterThan(const A& a, const B& b, const char* format, ...) {
    if (!(a > b)) {
      va_list args;
      std::string message;
      __MT_TO_STRING(format, args, &message);
      throw std::runtime_error(message);
    }
  }

  template <typename A, typename B>
  static void isGreaterEqual(const A& a, const B& b, const char* format, ...) {
    if (!(a >= b)) {
      va_list args;
      std::string message;
      __MT_TO_STRING(format, args, &message);
      throw std::runtime_error(message);
    }
  }

  Check() = delete;
};

}  // namespace mt

#endif  // MT_CHECK_H_
