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

#ifndef MULTIMAP_GENERATOR_HPP
#define MULTIMAP_GENERATOR_HPP

#include <limits>
#include <random>
#include <string>

namespace multimap {
namespace internal {

typedef std::uint16_t S;  // min/max/avg = 1/ 6/ 3.5 bytes.
typedef std::uint32_t M;  // min/max/avg = 1/10/ 5.5 bytes.
typedef std::uint64_t L;  // min/max/avg = 1/20/10.5 bytes.

template <typename Size>
class Generator {
 public:
  static const std::size_t min_size;
  static const std::size_t max_size;
  static const std::size_t avg_size;

  // Creates a Generator that is able to produce an evenly distributed
  // sequence of 'num' unique byte patterns of [min, max] bytes in size.
  Generator(std::size_t num_unique);

  std::string Generate() const;

 private:
  std::size_t num_unique_;
  std::default_random_engine engine_;
  std::uniform_int_distribution<Size> distribution_;
};

template <typename Size>
const std::size_t Generator<Size>::min_size(
    std::to_string(std::numeric_limits<Size>::min()).size());

template <typename Size>
const std::size_t Generator<Size>::max_size(
    std::to_string(std::numeric_limits<Size>::max()).size());

template <typename Size>
const std::size_t Generator<Size>::avg_size((max_size - min_size) / 2);

template <typename Size>
Generator<Size>::Generator(std::size_t num_unique)
    : num_unique_(num_unique) {}

template <typename Size>
std::string Generator<Size>::Generate() const {
  return std::to_string(distribution_(engine_) % num_unique_);
}

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_GENERATOR_HPP
