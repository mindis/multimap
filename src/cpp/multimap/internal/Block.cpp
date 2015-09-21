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

#include "multimap/internal/Block.hpp"

#include <cassert>
#include <cstring>
#include <array>
#include "multimap/internal/System.hpp"
#include "multimap/internal/thirdparty/mt.hpp"

namespace multimap {
namespace internal {

Block::Block() : data_(nullptr), size_(0), position_(0) {}

Block::Block(void* data, std::uint32_t size) { setData(data, size); }

double Block::load_factor() const {
  return (size_ != 0) ? (static_cast<double>(position_) / size_) : 0;
}

std::uint32_t Block::max_value_size() const {
  return (size_ > SIZE_OF_VALUE_SIZE_FIELD) ? (size_ - SIZE_OF_VALUE_SIZE_FIELD)
                                            : 0;
}

bool Block::add(const Bytes& value) {
  assert(data_);
  mt::check(value.size() <= max_value_size(),
        "Block: Reject value because its size of %u bytes exceeds the allowed "
        "maximum of %u bytes.",
        value.size(), max_value_size());
  const std::int16_t value_size = value.size();
  const auto num_bytes_required = sizeof value_size + value_size;
  if (num_bytes_required > num_bytes_free()) {
    return false;
  }
  // For little-endian only.
  unsigned char* ptr = reinterpret_cast<unsigned char*>(data_) + position_;
  *(ptr++) = value_size >> 8;
  *(ptr++) = value_size;
  std::memcpy(ptr, value.data(), value_size);
  position_ += sizeof value_size;
  position_ += value_size;
  return true;
}

bool operator==(const Block& lhs, const Block& rhs) {
  if (lhs.data() == rhs.data()) return true;
  return (lhs.size() == rhs.size())
             ? std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0
             : false;
}

}  // namespace internal
}  // namespace multimap
