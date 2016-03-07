// This file is part of Multimap.  http://multimap.io
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

#ifndef MULTIMAP_INTERNAL_GENERATOR_HPP_INCLUDED
#define MULTIMAP_INTERNAL_GENERATOR_HPP_INCLUDED

#include <limits>
#include <memory>
#include <random>
#include <string>

namespace multimap {
namespace internal {

class Generator {
 public:
  virtual void reset() = 0;

  virtual std::string next() = 0;

  std::string nextof(size_t size) {
    std::string bytes = this->next();
    if (bytes.size() != size) {
      bytes.resize(size, 'x');
    }
    return bytes;
  }
};

class RandomGenerator : public Generator {
 public:
  RandomGenerator() = default;

  RandomGenerator(size_t num_unique) : num_unique_(num_unique) {}

  void reset() override { random_engine_ = std::default_random_engine(); }

  std::string next() override {
    return std::to_string(distribution_(random_engine_) % num_unique_);
  }

  size_t numUnique() const { return num_unique_; }

 private:
  std::default_random_engine random_engine_;
  std::uniform_int_distribution<std::uint64_t> distribution_;
  const size_t num_unique_ = std::numeric_limits<size_t>::max();
};

class SequenceGenerator : public Generator {
 public:
  SequenceGenerator() = default;

  SequenceGenerator(size_t first_value) : first_value_(first_value), current_value_(first_value) {}

  void reset() override { current_value_ = first_value_; }

  std::string next() override { return std::to_string(current_value_++); }

  size_t firstValue() const { return first_value_; }

 private:
  const size_t first_value_ = 0;
  size_t current_value_ = 0;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_GENERATOR_HPP_INCLUDED
