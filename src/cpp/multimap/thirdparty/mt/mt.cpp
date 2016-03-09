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

#include "mt/mt.hpp"

#include <cxxabi.h>
#include <execinfo.h>
#include <array>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
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
    if (std::any_of(key.begin(), key.end(), is_space)) continue;
    if (std::any_of(val.begin(), val.end(), is_space)) continue;
    if (key.empty() || val.empty()) continue;
    joined.append(key);
    joined.push_back('=');
    joined.append(val);
    joined.push_back('\n');
  }
  return joined;
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

std::string makeErrorMessage(const char* file, unsigned line,
                             const char* message,
                             size_t skip_head_from_stacktrace) {
  std::ostringstream oss;
  oss << "Fatal error in " << file << ':' << line
      << "\nwith message: " << message << "\n\n";
  internal::printStackTraceTo(oss, skip_head_from_stacktrace);
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
  internal::printStackTraceTo(oss, skip_head_from_stacktrace);
  return oss.str();
}

}  // namespace

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
  const auto property = "VmRSS:";
  const auto filename = "/proc/self/status";
  std::ifstream input(filename);
  check(input.is_open(), "Could not open '%s'", filename);
  std::string token;
  uint64_t mem_in_kb;
  while (input >> token) {
    if (token == property) {
      input >> mem_in_kb;
      return KiB(mem_in_kb);
    }
  }
  fail("No '%s' property found in '%s'", property, filename);
  return 0;
}

uint32_t crc32(const std::string& str) { return crc32(str.data(), str.size()); }

uint32_t crc32(const void* data, size_t size) {
  boost::crc_32_type crc;
  crc.process_bytes(data, size);
  return crc.checksum();
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
  const auto time_since_epoch = std::time(nullptr);
  Check::notEqual(time_since_epoch, -1, "std::time() failed");
  struct tm broken_down_time;
  ::localtime_r(&time_since_epoch, &broken_down_time);

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

const int AutoCloseFd::INVALID = -1;

AutoCloseFd& AutoCloseFd::operator=(AutoCloseFd&& other) {
  if (&other != this) {
    reset(other.fd_);
    other.fd_ = INVALID;
  }
  return *this;
}

void AutoCloseFd::reset(int fd) {
  if (fd_ != INVALID) {
    Check::isZero(::close(fd_), "close() failed because of '%s'", errnostr());
  }
  fd_ = fd;
}

const char* DirectoryLockGuard::DEFAULT_FILENAME = ".lock";

DirectoryLockGuard::DirectoryLockGuard(const boost::filesystem::path& directory,
                                       const std::string& filename)
    : directory_(directory), filename_(filename) {
  Check::isTrue(boost::filesystem::is_directory(directory),
                "No such directory '%s'", directory.c_str());
  const auto lock_filename = boost::filesystem::path(directory) / filename;
  Check::isFalse(
      boost::filesystem::exists(lock_filename),
      "Could not lock directory, because the lock file '%s' already exists",
      lock_filename.c_str());
  std::ofstream lock_file(lock_filename.string());
  Check::isTrue(lock_file.is_open(),
                "Could not create lock file '%s' for unknown reason",
                lock_filename.c_str());
  lock_file << ::getpid();
}

DirectoryLockGuard::~DirectoryLockGuard() {
  if (!directory_.empty()) {
    boost::filesystem::remove(boost::filesystem::path(directory_) / filename_);
  }
}

std::vector<boost::filesystem::path> Files::list(
    const boost::filesystem::path& directory) {
  const auto is_hidden = [](const boost::filesystem::path& path) {
    return path.empty() ? false : (path.filename().string().front() == '.');
  };

  std::vector<boost::filesystem::path> files;
  boost::filesystem::directory_iterator end;
  for (boost::filesystem::directory_iterator it(directory); it != end; ++it) {
    const auto& path = it->path();
    if (boost::filesystem::is_regular_file(path) && !is_hidden(path)) {
      files.push_back(path);
    }
  }
  return files;
}

Files::Bytes Files::readAllBytes(const boost::filesystem::path& filename) {
  Bytes bytes(boost::filesystem::file_size(filename));
  const auto fd = open(filename, O_RDONLY);
  read(fd.get(), bytes.data(), bytes.size());
  return bytes;
}

std::vector<std::string> Files::readAllLines(
    const boost::filesystem::path& filename) {
  std::ifstream input(filename.string());
  check(input.is_open(), "Could not open '%s'", filename.c_str());

  std::string line;
  std::vector<std::string> lines;
  while (std::getline(input, line)) {
    lines.push_back(line);
  }
  return lines;
}

Properties readPropertiesFromFile(const boost::filesystem::path& filename) {
  std::ifstream input(filename.string());
  check(input.is_open(), "Could not open '%s'", filename.c_str());

  std::string line;
  Properties properties;
  while (std::getline(input, line)) {
    if (line.empty()) continue;
    const auto pos_of_delim = line.find('=');
    if (pos_of_delim == std::string::npos) continue;
    const auto key = line.substr(0, pos_of_delim);
    const auto value = line.substr(pos_of_delim + 1);
    // We don't make any checks here, because external modification
    // of key or value will be captured during checksum verification.
    properties[key] = value;
  }

  check(properties.count("checksum") != 0,
        "Properties file '%s' is missing checksum.", filename.c_str());
  const auto actual_checksum = std::stoul(properties.at("checksum"));

  properties.erase("checksum");
  const auto expected_checksum = crc32(serializeToString(properties));
  check(actual_checksum == expected_checksum, "'%s' has wrong checksum.",
        filename.c_str());
  return properties;
}

void writePropertiesToFile(const boost::filesystem::path& filename,
                           const Properties& properties) {
  MT_REQUIRE_EQ(properties.count("checksum"), 0);

  std::ofstream output(filename.string());
  check(output.is_open(), "Could not create '%s'", filename.c_str());

  const auto content = serializeToString(properties);
  output << content << "checksum=" << crc32(content) << '\n';
}

namespace internal {

std::string toString(const char* format, va_list args) {
  std::string result;
  toString(format, args, &result);
  return result;
}

void toString(const char* format, va_list args, std::string* output) {
  va_list args2;
  va_copy(args2, args);
  std::vector<char> buf(std::vsnprintf(nullptr, 0, format, args) + 1);
  std::vsnprintf(buf.data(), buf.size(), format, args2);
  va_end(args2);
  output->assign(buf.data(), buf.size());
}

std::vector<std::string> getStackTrace(size_t skip_head) {
  std::vector<std::string> result;
  std::array<void*, 23> frames;
  const auto size = backtrace(frames.data(), frames.size());
  if (static_cast<size_t>(size) > skip_head) {
    const auto symbols = backtrace_symbols(frames.data(), size);
    result.assign(symbols + skip_head, symbols + size);
    std::for_each(result.begin(), result.end(), demangle);
    std::free(symbols);
  }
  return result;
}

void printStackTraceTo(std::ostream& os, size_t skip_head) {
  const auto stacktrace = getStackTrace(skip_head);
  std::copy(stacktrace.begin(), stacktrace.end(),
            std::ostream_iterator<std::string>(os, "\n"));
}

void printStackTrace(size_t skip_head) {
  printStackTraceTo(std::cerr, skip_head);
}

}  // namespace internal

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

}  // namespace mt
