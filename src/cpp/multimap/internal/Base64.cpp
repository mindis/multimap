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

#include "multimap/internal/Base64.hpp"

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

namespace multimap {
namespace internal {

using namespace boost::archive::iterators;
typedef base64_from_binary<transform_width<const char*, 6, 8>> ToBase64Iter;
typedef transform_width<binary_from_base64<const char*>, 8, 6> ToBinaryIter;

void Base64::encode(const Bytes& binary, std::string* base64) {
  assert(binary.data());
  assert(base64);

  base64->assign(ToBase64Iter(binary.data()),
                 ToBase64Iter(binary.data() + binary.size()));

  // Boost does not handle Base64 padding.
  const auto num_padding_chars = (3 - (binary.size() % 3)) % 3;
  base64->append(num_padding_chars, '=');
}

void Base64::decode(const std::string& base64, std::string* binary) {
  assert(binary);

  // Boost does not handle Base64 padding.
  auto base64_copy = base64;
  auto iter = base64_copy.rbegin();
  while (iter != base64_copy.rend() && *iter == '=') {
    *iter = 'A';
    ++iter;
  }

  binary->assign(ToBinaryIter(base64_copy.data()),
                 ToBinaryIter(base64_copy.data() + base64.size()));
  binary->resize(binary->size() - std::distance(base64_copy.rbegin(), iter));
}

}  // namespace internal
}  // namespace multimap
