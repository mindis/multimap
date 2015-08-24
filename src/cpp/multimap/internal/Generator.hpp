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

#include <limits>
#include <memory>
#include <random>
#include <string>

namespace multimap {
namespace internal {

class Generator {
 public:
  virtual std::string Generate() = 0;

  std::string Generate(std::size_t min_size) {
    auto value = this->Generate();
    if (value.size() < min_size) {
      value.append(min_size - value.size(), 'x');
    }
    return value;
  }

  virtual void Reset() = 0;
};

class RandomGenerator : public Generator {
 public:
  RandomGenerator() : num_unique_(std::numeric_limits<std::size_t>::max()) {}

  RandomGenerator(std::size_t num_unique) : num_unique_(num_unique) {}

  static std::unique_ptr<Generator> New() {
    return std::unique_ptr<Generator>(new RandomGenerator());
  }

  static std::unique_ptr<Generator> New(std::size_t num_unique) {
    return std::unique_ptr<Generator>(new RandomGenerator(num_unique));
  }

  std::string Generate() override {
    return std::to_string(distribution_(random_engine_) % num_unique_);
  }

  void Reset() override { random_engine_ = std::default_random_engine(); }

  std::size_t num_unique() const { return num_unique_; }

 private:
  const std::size_t num_unique_;
  std::default_random_engine random_engine_;
  std::uniform_int_distribution<std::uint64_t> distribution_;
};

class SequenceGenerator : public Generator {
 public:
  SequenceGenerator() : SequenceGenerator(0) {}

  SequenceGenerator(std::size_t start) : start_(start), state_(start) {}

  static std::unique_ptr<Generator> New() {
    return std::unique_ptr<Generator>(new SequenceGenerator());
  }

  static std::unique_ptr<Generator> New(std::size_t start) {
    return std::unique_ptr<Generator>(new SequenceGenerator(start));
  }

  std::string Generate() override { return std::to_string(state_++); }

  void Reset() override { state_ = start_; }

  std::size_t start() const { return start_; }

 private:
  const std::size_t start_;
  std::size_t state_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_GENERATOR_HPP
