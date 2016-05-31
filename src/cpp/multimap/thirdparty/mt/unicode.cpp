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

#include "mt/unicode.h"

#include <cstring>
#include <cwchar>
#include <cwctype>
#include <string>
#include "mt/check.h"

// https://www.cl.cam.ac.uk/~mgk25/unicode.html
// http://www.gnu.org/software/libc/manual/html_node/Character-Set-Handling.html

static_assert(sizeof(wchar_t) == 4,
              "Unicode handling is not supported on this platform because the "
              "wchar_t type cannot hold 32-bit codepoints.");

namespace mt {

void setUtf8Locale() {
  const char* locale = "C.UTF-8";
  Check::notNull(std::setlocale(LC_ALL, locale),
                 "std::setlocale(LC_ALL, \"%s\" failed", locale);
}

void toUtf8(const std::wstring& utf32, std::string* utf8) {
  std::mbstate_t state;
  std::memset(&state, 0, sizeof state);
  utf8->resize(utf32.size() * MB_CUR_MAX);
  const wchar_t* utf32_begin = utf32.c_str();
  const size_t status = std::wcsrtombs(const_cast<char*>(utf8->data()),
                                       &utf32_begin, utf8->size(), &state);
  // C++11 requires that elements of std::string are stored contiguously,
  // so casting away constness and writing directly into the array is safe.
  // C++17 adds a non-const version of std::string::data() for that reason.
  mt::check(status != static_cast<size_t>(-1),
            "std::wcsrtombs() failed because of '%s'", mt::errnostr());
  utf8->resize(status);
}

std::string toUtf8Copy(const std::wstring& utf32) {
  std::string result;
  toUtf8(utf32, &result);
  return result;
}

void toUtf32(const std::string& utf8, std::wstring* utf32) {
  std::mbstate_t state;
  std::memset(&state, 0, sizeof state);
  utf32->resize(utf8.size());
  const char* utf8_begin = utf8.c_str();
  const size_t status = std::mbsrtowcs(const_cast<wchar_t*>(utf32->data()),
                                       &utf8_begin, utf32->size(), &state);
  // C++11 requires that elements of std::wstring are stored contiguously,
  // so casting away constness and writing directly into the array is safe.
  // C++17 adds a non-const version of std::wstring::data() for that reason.
  mt::check(status != static_cast<size_t>(-1),
            "std::mbsrtowcs() failed because of '%s'", mt::errnostr());
  utf32->resize(status);
}

std::wstring toUtf32Copy(const std::string& utf8) {
  std::wstring result;
  toUtf32(utf8, &result);
  return result;
}

void toLowerUtf8(std::string* utf8) {
  thread_local std::wstring utf32;
  toUtf32(*utf8, &utf32);
  toLowerUtf32(&utf32);
  toUtf8(utf32, utf8);
}

std::string toLowerUtf8Copy(const std::string& utf8) {
  std::string result = utf8;
  toLowerUtf8(&result);
  return result;
}

void toLowerUtf32(std::wstring* utf32) {
  for (wchar_t& ch : *utf32) {
    ch = std::towlower(ch);
  }
}

std::wstring toLowerUtf32Copy(const std::wstring& utf32) {
  std::wstring result = utf32;
  toLowerUtf32(&result);
  return result;
}

void toUpperUtf8(std::string* utf8) {
  thread_local std::wstring utf32;
  toUtf32(*utf8, &utf32);
  toUpperUtf32(&utf32);
  toUtf8(utf32, utf8);
}

std::string toUpperUtf8Copy(const std::string& utf8) {
  std::string result = utf8;
  toUpperUtf8(&result);
  return result;
}

void toUpperUtf32(std::wstring* utf32) {
  for (wchar_t& ch : *utf32) {
    ch = std::towupper(ch);
  }
}

std::wstring toUpperUtf32Copy(const std::wstring& utf32) {
  std::wstring result = utf32;
  toUpperUtf32(&result);
  return result;
}

}  // namespace mt
