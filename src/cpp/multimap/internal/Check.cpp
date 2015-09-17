// This file is part of the Multimap library.  http://multimap.io
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

#include "multimap/internal/Check.hpp"

#include <cstdarg>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace multimap {
namespace internal {

namespace {

void check(bool expression, const char* format, va_list args) {
  if (!expression) {
    va_list args2;
    va_copy(args2, args);
    const auto num_bytes = std::vsnprintf(nullptr, 0, format, args) + 1;
    std::vector<char> buffer(num_bytes);
    std::vsnprintf(buffer.data(), buffer.size(), format, args2);
    va_end(args2);
    throw std::runtime_error(buffer.data());
  }
}

}  // namespace

void check(bool expression, const char* format, ...) {
  va_list args;
  va_start(args, format);
  check(expression, format, args);
  va_end(args);
}

}  // namespace internal
}  // namespace multimap
