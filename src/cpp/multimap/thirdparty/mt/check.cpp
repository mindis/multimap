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

#include "mt/check.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace mt {
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

}  // namespace internal

const char* errnostr() { return std::strerror(errno); }

}  // namespace mt
