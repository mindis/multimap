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
#include "multimap/thirdparty/mt/common.hpp"
#include "multimap/Bytes.hpp"

namespace multimap {
namespace internal {

class UintVector {
 public:
  UintVector() = default;

  void add(uint32_t value);

  std::vector<uint32_t> unpack() const;

  bool empty() const { return offset_ == 0; }

  static UintVector readFromStream(std::FILE* stream);

  void writeToStream(std::FILE* stream) const;

 private:
  void allocateMoreIfFull();

  byte* begin() const { return data_.get(); }

  byte* end() const { return data_.get() + size_; }

  byte* cur() const { return data_.get() + offset_; }

  std::unique_ptr<byte[]> data_;
  uint32_t offset_ = 0;
  uint32_t size_ = 0;
};

MT_ASSERT_SIZEOF(UintVector, 12, 16);

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_UINT_VECTOR_HPP_INCLUDED
