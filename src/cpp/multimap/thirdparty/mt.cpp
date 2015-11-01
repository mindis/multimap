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

#include <cxxabi.h>
#include <execinfo.h>
#include <array>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <memory>
#include <boost/crc.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/operations.hpp>

namespace mt {

namespace {

std::string serializeToString(const Properties& properties) {
  std::string joined;
  const auto is_space = [](char c) { return std::isspace(c); };
  for (const auto& entry : properties) {
    const auto key = boost::trim_copy(entry.first);
    const auto val = boost::trim_copy(entry.second);
    if (std::any_of(key.begin(), key.end(), is_space))
      continue;
    if (std::any_of(val.begin(), val.end(), is_space))
      continue;
    if (key.empty() || val.empty())
      continue;
    joined.append(key);
    joined.push_back('=');
    joined.append(val);
    joined.push_back('\n');
  }
  return joined;
}

void throwRuntimeError3(const char* format, va_list args) {
  va_list args2;
  va_copy(args2, args);
  const auto num_bytes = std::vsnprintf(nullptr, 0, format, args) + 1;
  std::vector<char> buffer(num_bytes);
  std::vsnprintf(buffer.data(), buffer.size(), format, args2);
  va_end(args2);
  throwRuntimeError(buffer.data());
}

void demangle(std::string& symbol) {
  const auto stripFunctionName = [](std::string& symbol) {
    const auto pos_lparen = symbol.find('(');
    const auto pos_rparen = symbol.find(')');
    if (pos_lparen < pos_rparen) {
      symbol = symbol.substr(pos_lparen + 1, pos_rparen - pos_lparen - 1);
    }
  };

  const auto stripAddress = [](std::string& symbol) {
    const auto pos_plus = symbol.find('+');
    if (pos_plus != std::string::npos) {
      symbol = symbol.substr(0, pos_plus);
    }
  };

  int status;
  stripFunctionName(symbol);
  stripAddress(symbol);
  boost::trim(symbol);
  if (symbol.empty()) {
    symbol.assign("== inlined function ==");
  } else if (boost::starts_with(symbol, "_Z")) {
    std::unique_ptr<char, decltype(std::free)*> realname(
        abi::__cxa_demangle(symbol.c_str(), nullptr, nullptr, &status),
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
        symbol.assign(realname.get());
    }
  }
}

std::string makeErrorMessage(const char* file, unsigned line, const char* expr,
                             AssertionError::Expected expected,
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
  printStackTraceTo(oss, skip_head_from_stacktrace);
  return oss.str();
}

} // namespace

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

std::size_t crc32(const std::string& str) {
  return crc32(str.data(), str.size());
}

std::size_t crc32(const char* data, std::size_t size) {
  boost::crc_32_type crc;
  crc.process_bytes(data, size);
  return crc.checksum();
}

// Source: http://www.isthe.com/chongo/src/fnv/hash_32a.c
std::uint32_t fnv1aHash32(const void* buf, std::size_t len) {
  std::uint32_t hash = 0x811c9dc5; // FNV1_32A_INIT
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
  std::uint64_t hash = 0xcbf29ce484222325ULL; // FNV1A_64_INIT
  const auto uchar_buf = reinterpret_cast<const unsigned char*>(buf);
  for (std::size_t i = 0; i != len; ++i) {
    hash ^= uchar_buf[i];
    hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) +
            (hash << 8) + (hash << 40);
  }
  return hash;
}

Files::Bytes Files::readAllBytes(const boost::filesystem::path& filepath) {
  std::ifstream ifs(filepath.string());
  mt::check(ifs, Messages::COULD_NOT_OPEN, filepath.c_str());

  Bytes bytes(boost::filesystem::file_size(filepath));
  ifs.read(bytes.data(), bytes.size());
  MT_ENSURE_EQ(static_cast<std::size_t>(ifs.gcount()), bytes.size());
  return bytes;
}

std::vector<std::string> Files::readAllLines(
    const boost::filesystem::path& filepath) {
  std::ifstream ifs(filepath.string());
  check(ifs, Messages::COULD_NOT_OPEN, filepath.c_str());

  std::string line;
  std::vector<std::string> lines;
  while (std::getline(ifs, line)) {
    lines.push_back(line);
  }
  return lines;
}

AutoCloseFile::AutoCloseFile(std::FILE* file) : file_(file) {}

AutoCloseFile::~AutoCloseFile() { reset(); }

AutoCloseFile::AutoCloseFile(AutoCloseFile&& other) : file_(other.file_) {
  other.file_ = nullptr;
}

AutoCloseFile& AutoCloseFile::operator=(AutoCloseFile&& other) {
  if (&other != this) {
    reset(other.file_);
    other.file_ = nullptr;
  }
  return *this;
}

std::FILE* AutoCloseFile::get() const { return file_; }

void AutoCloseFile::reset(std::FILE* file) {
  if (file_) {
    const auto status = std::fclose(file_);
    MT_ASSERT_IS_ZERO(status);
  }
  file_ = file;
}

Properties readPropertiesFromFile(const std::string& filepath) {
  std::ifstream ifs(filepath);
  check(ifs, Messages::COULD_NOT_OPEN, filepath.c_str());

  std::string line;
  Properties properties;
  while (std::getline(ifs, line)) {
    if (line.empty())
      continue;
    const auto pos_of_delim = line.find('=');
    if (pos_of_delim == std::string::npos)
      continue;
    const auto key = line.substr(0, pos_of_delim);
    const auto value = line.substr(pos_of_delim + 1);
    // We don't make any checks here, because external modification
    // of key or value will be captured during checksum verification.
    properties[key] = value;
  }

  check(properties.count("checksum") != 0,
        "Properties file '%s' is missing checksum.", filepath.c_str());
  const auto actual_checksum = std::stoul(properties.at("checksum"));

  properties.erase("checksum");
  const auto expected_checksum = crc32(serializeToString(properties));
  check(actual_checksum == expected_checksum, "'%s' has wrong checksum.",
        filepath.c_str());
  return properties;
}

void writePropertiesToFile(const Properties& properties,
                           const std::string& filepath) {
  MT_REQUIRE_EQ(properties.count("checksum"), 0);

  std::ofstream ofs(filepath);
  check(ofs, Messages::COULD_NOT_CREATE, filepath.c_str());

  const auto content = serializeToString(properties);
  ofs << content << "checksum=" << crc32(content) << '\n';
}

void throwRuntimeError(const char* message) {
  throw std::runtime_error(message);
}

void throwRuntimeError2(const char* format, ...) {
  va_list args;
  va_start(args, format);
  throwRuntimeError3(format, args);
  va_end(args);
}

void check(bool expression, const char* format, ...) {
  if (!expression) {
    va_list args;
    va_start(args, format);
    throwRuntimeError3(format, args);
    va_end(args);
  }
}

std::vector<std::string> getStackTrace(std::size_t skip_head) {
  std::vector<std::string> result;
  std::array<void*, 23> frames;
  const auto size = backtrace(frames.data(), frames.size());
  if (static_cast<std::size_t>(size) > skip_head) {
    const auto symbols = backtrace_symbols(frames.data(), size);
    result.assign(symbols + skip_head, symbols + size);
    std::for_each(result.begin(), result.end(), demangle);
    std::free(symbols);
  }
  return result;
}

void printStackTraceTo(std::ostream& os, std::size_t skip_head) {
  const auto stacktrace = getStackTrace(skip_head);
  std::copy(stacktrace.begin(), stacktrace.end(),
            std::ostream_iterator<std::string>(os, "\n"));
}

void printStackTrace(std::size_t skip_head) {
  printStackTraceTo(std::cerr, skip_head);
}

AssertionError::AssertionError(const char* message)
    : std::runtime_error(message) {}

AssertionError::AssertionError(const std::string& message)
    : std::runtime_error(message) {}

AssertionError::AssertionError(const char* file, std::size_t line,
                               const char* expr,
                               AssertionError::Expected expected,
                               AssertionError::Type type)
    : std::runtime_error(
          makeErrorMessage(file, line, expr, expected, type, 5)) {}

} // namespace mt
