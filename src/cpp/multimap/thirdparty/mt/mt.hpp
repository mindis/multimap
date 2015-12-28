// This file is part of the MT library.
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

#define _FILE_OFFSET_BITS 64
// Enables large file support (> 2 GiB) on 32-bit systems.

// TODO Add feature test macros

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <iostream>
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

static const uint32_t VERSION = 20151228;

// -----------------------------------------------------------------------------
// COMMON
// -----------------------------------------------------------------------------

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

struct Resource {
  // Types representing resources are not copyable or moveable.
  // To enforce this rule such types should derive from this class.

  Resource() = default;
  Resource(const Resource&) = delete;
  Resource& operator=(const Resource&) = delete;

  // Move construction and assignment are implicitly disabled.
};

// -----------------------------------------------------------------------------
// ALGORITHM
// -----------------------------------------------------------------------------

uint32_t crc32(const std::string& str);
// Computes and returns the 32-bit CRC checksum for `str`.

uint32_t crc32(const void* data, size_t size);
// Computes and returns the 32-bit CRC checksum for `[data, data + size)`.

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
// ERROR HANDLING
// -----------------------------------------------------------------------------

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

struct Check {
  // TODO Make descriptive error messages.

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

inline const char* errnostr() { return std::strerror(errno); }

// -----------------------------------------------------------------------------
// INPUT / OUTPUT
// -----------------------------------------------------------------------------

class AutoCloseFd {
 public:
  AutoCloseFd() = default;

  explicit AutoCloseFd(int fd) : fd_(fd) {}

  AutoCloseFd(AutoCloseFd&& other) : fd_(other.fd_) { other.fd_ = -1; }

  ~AutoCloseFd() { reset(); }

  AutoCloseFd& operator=(AutoCloseFd&& other) {
    if (&other != this) {
      reset(other.fd_);
      other.fd_ = -1;
    }
    return *this;
  }

  int get() const { return fd_; }

  void reset(int fd = -1) {
    if (fd_ != -1) {
      const auto result = ::close(fd_);
      Check::isZero(result, "::close() failed because of '%s'", errnostr());
    }
    fd_ = fd;
  }

 private:
  int fd_ = -1;
};

inline AutoCloseFd open(const boost::filesystem::path& file, int flags) {
  const auto result = ::open(file.c_str(), flags);
  Check::notEqual(result, -1, "mt::open() failed for '%s' because of '%s'",
                  file.c_str(), errnostr());
  return AutoCloseFd(result);
}

inline AutoCloseFd open(const boost::filesystem::path& file, int flags,
                        mode_t mode) {
  const auto result = ::open(file.c_str(), flags, mode);
  Check::notEqual(result, -1, "mt::open() failed for '%s' because of '%s'",
                  file.c_str(), errnostr());
  return AutoCloseFd(result);
}

inline AutoCloseFd create(const boost::filesystem::path& file, mode_t mode) {
  const auto result = ::creat(file.c_str(), mode);
  Check::notEqual(result, -1, "mt::create() failed for '%s' because of '%s'",
                  file.c_str(), errnostr());
  return AutoCloseFd(result);
}

inline void read(int fd, void* buf, size_t len) {
  const auto result = ::read(fd, buf, len);
  Check::notEqual(result, -1, "mt::read() failed because of '%s'", errnostr());
  Check::isEqual(static_cast<size_t>(result), len,
                 "mt::read() read less bytes than expected");
}

inline void write(int fd, const void* buf, size_t len) {
  const auto result = ::write(fd, buf, len);
  Check::notEqual(result, -1, "mt::write() failed because of '%s'", errnostr());
  Check::isEqual(static_cast<size_t>(result), len,
                 "mt::write() wrote less bytes than expected");
}

inline void writeOrPrompt(int fd, const void* buf, size_t len) {
  while (true) {
    const auto result = ::write(fd, buf, len);
    Check::notEqual(result, -1, "mt::write() failed because of '%s'",
                    errnostr());
    if (static_cast<size_t>(result) < len) {
      std::cout << "Write operation failed because only " << result << " of "
                << len << " bytes could be written.\nIn case you ran out of "
                          "disk space you can do some cleanup now and then "
                          "try to continue.\nTry to continue? [Y/n] "
                << std::flush;
      std::string answer;
      std::getline(std::cin, answer);
      if (answer == "n") {
        fail("mt::write() wrote less bytes than expected");
      }
      buf = static_cast<const char*>(buf) + result;
      len -= result;
    } else {
      break;
    }
  }
}

inline uint64_t seek(int fd, std::int64_t offset, int whence) {
  const auto result = ::lseek(fd, offset, whence);
  Check::notEqual(result, -1, "mt::seek() failed because of '%s'", errnostr());
  return result;
}

inline uint64_t tell(int fd) { return seek(fd, 0, SEEK_CUR); }

inline void truncate(int fd, uint64_t length) {
  const auto result = ::ftruncate(fd, length);
  Check::isZero(result, "mt::truncate() failed because of '%s'", errnostr());
}

inline void* mmap(void* addr, uint64_t length, int prot, int flags, int fd,
                  off_t offset) {
  const auto result = ::mmap(addr, length, prot, flags, fd, offset);
  Check::notEqual(result, MAP_FAILED, "mt::mmap() failed because of '%s'",
                  errnostr());
  return result;
}

inline void* mremap(void* old_addr, uint64_t old_size, uint64_t new_size,
                    int flags) {
  const auto result = ::mremap(old_addr, old_size, new_size, flags);
  Check::notEqual(result, MAP_FAILED, "mt::mremap() failed because of '%s'",
                  errnostr());
  return result;
}

inline void munmap(void* addr, uint64_t length) {
  const auto result = ::munmap(addr, length);
  Check::isZero(result, "mt::munmap() failed because of '%s'", errnostr());
}

class AutoCloseFile {
 public:
  AutoCloseFile() = default;

  explicit AutoCloseFile(std::FILE* file) : file_(file) {}

  AutoCloseFile(AutoCloseFile&& other) : file_(other.file_) {
    other.file_ = nullptr;
  }

  ~AutoCloseFile() { reset(); }

  AutoCloseFile& operator=(AutoCloseFile&& other) {
    if (&other != this) {
      reset(other.file_);
      other.file_ = nullptr;
    }
    return *this;
  }

  std::FILE* get() const { return file_; }

  void reset(std::FILE* file = nullptr) {
    if (file_) {
      const auto status = std::fclose(file_);
      Check::isZero(status, "std::fclose() failed");
    }
    file_ = file;
  }

 private:
  std::FILE* file_ = nullptr;
};

inline AutoCloseFile fopen(const boost::filesystem::path& file,
                           const char* mode) {
  const auto result = std::fopen(file.c_str(), mode);
  Check::notNull(result, "mt::fopen() failed because of '%s'", errnostr());
  return AutoCloseFile(result);
}

inline void fread(std::FILE* stream, void* buf, uint64_t len) {
  const auto result = std::fread(buf, sizeof(char), len, stream);
  Check::isEqual(result, len, "mt::fread() failed");
}

inline void fwrite(std::FILE* stream, const void* buf, size_t len) {
  const auto result = std::fwrite(buf, sizeof(char), len, stream);
  Check::isEqual(result, len, "mt::fwrite() failed");
}

inline void fwriteOrPrompt(std::FILE* stream, const void* buf, size_t len) {
  while (true) {
    const auto result = std::fwrite(buf, sizeof(char), len, stream);
    if (result < len) {
      std::cout << "Write operation failed because only " << result << " of "
                << len << " bytes could be written.\nIn case you ran out of "
                          "disk space you can do some cleanup now and then "
                          "try to continue.\nTry to continue? [Y/n] "
                << std::flush;
      std::string answer;
      std::getline(std::cin, answer);
      if (answer == "n") {
        fail("mt::fwrite() wrote less bytes than expected");
      }
      buf = static_cast<const char*>(buf) + result;
      len -= result;
    } else {
      break;
    }
  }
}

inline void fseek(std::FILE* stream, long offset, int origin) {
  const auto result = std::fseek(stream, offset, origin);
  Check::isZero(result, "mt::fseek() failed");
}

inline uint64_t ftell(std::FILE* stream) {
  const auto result = std::ftell(stream);
  Check::notEqual(result, -1, "mt::ftell() failed because of '%s'", errnostr());
  return result;
}

class DirectoryLockGuard {
 public:
  static const char* DEFAULT_FILENAME;

  DirectoryLockGuard() = default;

  DirectoryLockGuard(const boost::filesystem::path& directory,
                     const std::string& filename = DEFAULT_FILENAME);

  DirectoryLockGuard(const DirectoryLockGuard&) = delete;
  DirectoryLockGuard& operator=(const DirectoryLockGuard&) = delete;

  DirectoryLockGuard(DirectoryLockGuard&& other);
  DirectoryLockGuard& operator=(DirectoryLockGuard&& other);

  ~DirectoryLockGuard();

  const boost::filesystem::path& directory() const;

  const std::string& filename() const;

 private:
  boost::filesystem::path directory_;
  std::string filename_;
};

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
    check(ofs, "Could not create '%s'", filepath.c_str());

    for (const auto& value : container) {
      print_to(value, ofs);
      ofs << '\n';
    }
  }
};

// -----------------------------------------------------------------------------
// CONFIGURATION
// -----------------------------------------------------------------------------

typedef std::map<std::string, std::string> Properties;

Properties readPropertiesFromFile(const std::string& filepath);

void writePropertiesToFile(const Properties& properties,
                           const std::string& filepath);

// -----------------------------------------------------------------------------
// TYPE TRAITS
// -----------------------------------------------------------------------------

template <typename T>
constexpr bool hasExpectedSize(size_t size_on_32_bit_system,
                               size_t size_on_64_bit_system) {
  return sizeof(T) ==
         (is32BitSystem() ? size_on_32_bit_system : size_on_64_bit_system);
}
// Checks at compile time if sizeof(T) has some expected value.
//
//   static_assert(mt::hasExpectedSize<Type>(40, 48)::value,
//                 "class Type does not have expected size");

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
// CONTRACT-BASED PROGRAMMING
// -----------------------------------------------------------------------------

class AssertionError : public std::logic_error {
 public:
  enum class Type { ASSERTION, PRECONDITION, POSTCONDITION };

  enum class Expected { TRUE, FALSE, IS_NULL, IS_ZERO, NOT_NULL, NOT_ZERO };

  explicit AssertionError(const char* message);

  explicit AssertionError(const std::string& message);

  AssertionError(const char* file, size_t line, const char* expr,
                 Expected expected, Type type = Type::ASSERTION);

  template <typename Lhs, typename Rhs>
  AssertionError(const char* file, size_t line, const char* expr, Lhs lhs_value,
                 Rhs rhs_value, Type type = Type::ASSERTION);
};

namespace internal {

std::vector<std::string> getStackTrace(size_t skip_head = 1);

void printStackTraceTo(std::ostream& os, size_t skip_head = 2);

void printStackTrace(size_t skip_head = 3);

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
  printStackTraceTo(oss, skip_head_from_stacktrace);
  return oss.str();
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

}  // namespace internal

template <typename Lhs, typename Rhs>
AssertionError::AssertionError(const char* file, size_t line, const char* expr,
                               Lhs lhs_value, Rhs rhs_value, Type type)
    : std::logic_error(internal::makeErrorMessage(file, line, expr, lhs_value,
                                                  rhs_value, type, 4)) {}

}  // namespace mt

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

// These assertions/macros are always enabled.

#define MT_ASSERT_TRUE(expr) __MT_ASSERT_TRUE(expr)
#define MT_ASSERT_FALSE(expr) __MT_ASSERT_FALSE(expr)
#define MT_ASSERT_NULL(expr) __MT_ASSERT_NULL(expr)
#define MT_ASSERT_ZERO(expr) __MT_ASSERT_ZERO(expr)
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
#define MT_REQUIRE_NULL(expr) __MT_REQUIRE_NULL(expr)
#define MT_REQUIRE_ZERO(expr) __MT_REQUIRE_ZERO(expr)
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
#define MT_ENSURE_NULL(expr) __MT_ENSURE_NULL(expr)
#define MT_ENSURE_ZERO(expr) __MT_ENSURE_ZERO(expr)
#define MT_ENSURE_NOT_NULL(expr) __MT_ENSURE_NOT_NULL(expr)
#define MT_ENSURE_NOT_ZERO(expr) __MT_ENSURE_NOT_ZERO(expr)
#define MT_ENSURE_EQ(a, b) __MT_ENSURE_COMPARE(a == b, a, b)
#define MT_ENSURE_NE(a, b) __MT_ENSURE_COMPARE(a != b, a, b)
#define MT_ENSURE_LT(a, b) __MT_ENSURE_COMPARE(a < b, a, b)
#define MT_ENSURE_LE(a, b) __MT_ENSURE_COMPARE(a <= b, a, b)
#define MT_ENSURE_GT(a, b) __MT_ENSURE_COMPARE(a > b, a, b)
#define MT_ENSURE_GE(a, b) __MT_ENSURE_COMPARE(a >= b, a, b)

#endif  // MT_MT_HPP_INCLUDED
