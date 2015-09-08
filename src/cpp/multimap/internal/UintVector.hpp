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

#ifndef MULTIMAP_INTERNAL_UINT_VECTOR_HPP
#define MULTIMAP_INTERNAL_UINT_VECTOR_HPP

#include <cstdio>
#include <cstdint>
#include <memory>
#include <vector>
#include "multimap/internal/Check.hpp"
#include "multimap/internal/Varint.hpp"

namespace multimap {
namespace internal {

class UintVector {
 public:
  UintVector();

  UintVector(const UintVector& other);

  UintVector& operator=(const UintVector& other);

  static UintVector ReadFromStream(std::FILE* stream);

  void WriteToStream(std::FILE* stream) const;

  std::vector<std::uint32_t> Unpack() const;

  void Add(std::uint32_t value);

  bool empty() const { return put_offset_ == 0; }

  void clear() {
    data_.reset();
    end_offset_ = 0;
    put_offset_ = 0;
  }

  static constexpr std::uint32_t max_value() {
    return Varint::max_value_4_bytes();
  }

 private:
  void AllocateMoreIfFull();

  std::unique_ptr<Varint::uchar[]> data_;
  std::uint32_t end_offset_;
  std::uint32_t put_offset_;
};

static_assert(HasExpectedSize<UintVector, 12, 16>::value,
              "class UintVector does not have expected size");

}  // internal
}  // multimap

#endif  // MULTIMAP_INTERNAL_UINT_VECTOR_HPP
