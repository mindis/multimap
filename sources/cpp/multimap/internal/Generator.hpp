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

#ifndef MULTIMAP_INTERNAL_GENERATOR_HPP
#define MULTIMAP_INTERNAL_GENERATOR_HPP

#include <random>
#include <string>

namespace multimap {
namespace internal {

class Generator {
 public:
  // Creates a Generator that is able to produce an evenly distributed
  // sequence of 'num_unique' byte patterns.
  Generator(std::size_t num_unique);

  std::string Generate();

  std::string Generate(std::size_t size);

  std::size_t num_unique() const { return num_unique_; }

 private:
  std::size_t num_unique_;
  std::default_random_engine engine_;
  std::uniform_int_distribution<std::uint64_t> distribution_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_GENERATOR_HPP
