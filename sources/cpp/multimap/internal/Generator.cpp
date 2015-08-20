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

#include "multimap/internal/Generator.hpp"
#include <cassert>

namespace multimap {
namespace internal {

Generator::Generator(std::size_t num_unique)
    : num_unique_(num_unique) {}

std::string Generator::Generate() {
  return std::to_string(distribution_(engine_) % num_unique_);
}

std::string Generator::Generate(std::size_t size) {
  auto str = Generate();
  const auto str_size = str.size();
  if (str_size < size) {
    str.append(size - str_size, 'x');
  } else if (str_size > size) {
    str.erase(size);
  }
  return str;
}

}  // namespace internal
}  // namespace multimap
