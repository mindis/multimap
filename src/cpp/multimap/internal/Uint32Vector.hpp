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

#ifndef MULTIMAP_INTERNAL_UINT_VECTOR_HPP_INCLUDED
#define MULTIMAP_INTERNAL_UINT_VECTOR_HPP_INCLUDED

#include <memory>
#include <vector>
#include "multimap/thirdparty/mt/mt.hpp"

namespace multimap {
namespace internal {

class UintVector {
 public:
  UintVector() = default;

  UintVector(UintVector&&) = default;
  UintVector& operator=(UintVector&&) = default;

  static UintVector readFromStream(std::FILE* stream);

  void writeToStream(std::FILE* stream) const;

  std::vector<uint32_t> unpack() const;

  bool add(uint32_t value);

  bool empty() const { return offset_ == 0; }

  void clear() {
    data_.reset();
    offset_ = 0;
    size_ = 0;
  }

 private:
  void allocateMoreIfFull();

  char* current() const { return data_.get() + offset_; }

  uint32_t remaining() const { return size_ - offset_; }

  std::unique_ptr<char[]> data_;
  uint32_t offset_ = 0;
  uint32_t size_ = 0;
};

static_assert(mt::hasExpectedSize<UintVector>(12, 16),
              "class UintVector does not have expected size");

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_UINT_VECTOR_HPP_INCLUDED
