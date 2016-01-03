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

#ifndef MULTIMAP_CALLABLES_HPP_INCLUDED
#define MULTIMAP_CALLABLES_HPP_INCLUDED

#include <algorithm>
#include "multimap/Bytes.hpp"

namespace multimap {

// -----------------------------------------------------------------------------
// Predicates
// -----------------------------------------------------------------------------

struct Equal {
  explicit Equal(const Bytes& bytes) : bytes_(bytes) {}

  bool operator()(const Bytes& bytes) const { return bytes == bytes_; }

 private:
  const Bytes bytes_;
};

struct Contains {
  explicit Contains(const Bytes& bytes) : bytes_(bytes) {}

  const Bytes& bytes() const { return bytes_; }

  bool operator()(const Bytes& bytes) const {
    return (bytes_.size() > 0)
               ? std::search(bytes.begin(), bytes.end(), bytes_.begin(),
                             bytes_.end()) != bytes.end()
               : true;
  }

 private:
  const Bytes bytes_;
};

struct StartsWith {
  explicit StartsWith(const Bytes& bytes) : bytes_(bytes) {}

  const Bytes& bytes() const { return bytes_; }

  bool operator()(const Bytes& value) const {
    return (value.size() >= bytes_.size())
               ? std::memcmp(value.data(), bytes_.data(), bytes_.size()) == 0
               : false;
  }

 private:
  const Bytes bytes_;
};

struct EndsWith {
  explicit EndsWith(const Bytes& bytes) : bytes_(bytes) {}

  const Bytes& bytes() const { return bytes_; }

  bool operator()(const Bytes& value) const {
    const int diff = value.size() - bytes_.size();
    return (diff >= 0)
               ? std::memcmp(value.data() + diff, bytes_.data(),
                             bytes_.size()) == 0
               : false;
  }

 private:
  const Bytes bytes_;
};

}  // namespace multimap

#endif  // MULTIMAP_CALLABLES_HPP_INCLUDED
