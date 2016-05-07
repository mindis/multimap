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

#include "mt/assert.h"

#include <cxxabi.h>    // abi::__cxa_demangle
#include <execinfo.h>  // backtrace and backtrace_symbols
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <boost/algorithm/string/predicate.hpp>  // NOLINT
#include <boost/algorithm/string/trim.hpp>       // NOLINT

namespace mt {
namespace {

// TODO(mtrenkmann): Refactor taking a const-reference and return a new string.
void demangle(std::string* symbol) {
  const auto strip_function_name = [](std::string* symbol) {
    const auto pos_lparen = symbol->find('(');
    const auto pos_rparen = symbol->find(')');
    if (pos_lparen < pos_rparen) {
      *symbol = symbol->substr(pos_lparen + 1, pos_rparen - pos_lparen - 1);
    }
  };

  const auto strip_address = [](std::string* symbol) {
    const auto pos_plus = symbol->find('+');
    if (pos_plus != std::string::npos) {
      *symbol = symbol->substr(0, pos_plus);
    }
  };

  int status;
  strip_function_name(symbol);
  strip_address(symbol);
  boost::trim(*symbol);
  if (symbol->empty()) {
    symbol->assign("== inlined function ==");
  } else if (boost::starts_with(*symbol, "_Z")) {
    std::unique_ptr<char, decltype(std::free)*> realname(
        abi::__cxa_demangle(symbol->c_str(), nullptr, nullptr, &status),
        std::free);
    switch (status) {
      case -1:
        std::cerr << "demangle: A memory allocation failure occurred.\n";
        break;
      case -2:
        std::cerr << "demangle: " << symbol
                  << " is not a valid name under the C++ ABI mangling rules.\n";
        break;
      case -3:
        std::cerr << "demagle: One of the arguments is invalid.\n";
        break;
      default:
        symbol->assign(realname.get());
    }
  }
}

std::string makeErrorMessage(const char* file, unsigned line,
                             const char* message,
                             size_t skip_head_from_stacktrace) {
  std::ostringstream oss;
  oss << "Fatal error in " << file << ':' << line
      << "\nwith message: " << message << "\n\n";
  internal::printStackTraceTo(&oss, skip_head_from_stacktrace);
  return oss.str();
}

std::string makeErrorMessage(const char* file, unsigned line, const char* expr,
                             AssertionError::Expected expected,
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
  switch (expected) {
    case AssertionError::Expected::TRUE:
      oss << "The expression '" << expr << "' should be true, but was false.";
      break;
    case AssertionError::Expected::FALSE:
      oss << "The expression '" << expr << "' should be false, but was true.";
      break;
    case AssertionError::Expected::IS_NULL:
      oss << "The expression '" << expr << "' doesn't yield nullptr.";
      break;
    case AssertionError::Expected::IS_ZERO:
      oss << "The expression '" << expr << "' doesn't yield zero.";
      break;
    case AssertionError::Expected::NOT_NULL:
      oss << "The expression '" << expr << "' yields nullptr.";
      break;
    case AssertionError::Expected::NOT_ZERO:
      oss << "The expression '" << expr << "' yields zero.";
      break;
    default:
      throw "The unexpected happened.";
  }
  oss << "\n\n";
  internal::printStackTraceTo(&oss, skip_head_from_stacktrace);
  return oss.str();
}

}  // namespace

AssertionError::AssertionError(const char* message)
    : std::logic_error(message) {}

AssertionError::AssertionError(const std::string& message)
    : std::logic_error(message) {}

AssertionError::AssertionError(const char* file, size_t line,
                               const char* message)
    : std::logic_error(makeErrorMessage(file, line, message, 5)) {}

AssertionError::AssertionError(const char* file, size_t line, const char* expr,
                               AssertionError::Expected expected,
                               AssertionError::Type type)
    : std::logic_error(makeErrorMessage(file, line, expr, expected, type, 5)) {}

namespace internal {

std::vector<std::string> getStackTrace(size_t skip_frames) {
  std::vector<std::string> result;
  std::array<void*, 23> frames;
  const auto size = backtrace(frames.data(), frames.size());
  if (static_cast<size_t>(size) > skip_frames) {
    const auto symbols = backtrace_symbols(frames.data(), size);
    result.assign(symbols + skip_frames, symbols + size);
    for (std::string& symbol : result) {
      demangle(&symbol);
    }
    std::free(symbols);
  }
  return result;
}

void printStackTraceTo(std::ostream* stream, size_t skip_frames) {
  const auto stacktrace = getStackTrace(skip_frames);
  std::copy(stacktrace.begin(), stacktrace.end(),
            std::ostream_iterator<std::string>(*stream, "\n"));
}

void printStackTrace(size_t skip_head) {
  printStackTraceTo(&std::cerr, skip_head);
}

}  // namespace internal
}  // namespace mt
