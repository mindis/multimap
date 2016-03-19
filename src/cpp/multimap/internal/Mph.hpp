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

#ifndef MULTIMAP_INTERNAL_MPH_HPP_INCLUDED
#define MULTIMAP_INTERNAL_MPH_HPP_INCLUDED

#include <memory>
#include <boost/filesystem/path.hpp>
#include "multimap/thirdparty/cmph/cmph.h"
#include "multimap/Slice.hpp"

namespace std {

template <>
struct default_delete<cmph_t> {
  void operator()(cmph_t* cmph) const {
    if (cmph) cmph_destroy(cmph);
  }
};

}  // namespace std

namespace multimap {
namespace internal {

class Mph {
  // This class is read-only and does not need external locking.

 public:
  // TODO Remove
  struct Options {
    bool verbose = true;
    CMPH_ALGO algorithm = CMPH_BDZ;
    uint32_t b = 0;  // 0 means use default value.
    double c = 0;    // 0 means use default value.

    Options() = default;
  };

  explicit Mph(const boost::filesystem::path& filename);

  Mph(Mph&&) = default;

  Mph& operator=(Mph&&) = default;

  static Mph build(const byte** keys, size_t nkeys, const Options& options);
  // A key in `keys` points to memory which is encoded like this:
  // [keylen as uint32][1 .. key data .. keylen]

  static Mph build(const boost::filesystem::path& keys, const Options& options);
  // `keys` contains a sequence of keys which are encoded as described above.

  void dump(const boost::filesystem::path& filename) const;

  size_t operator()(const Slice& key) const {
    return cmph_search(cmph_.get(), reinterpret_cast<const char*>(key.begin()),
                       key.size());
  }

  size_t size() const { return cmph_size(cmph_.get()); }

 private:
  explicit Mph(std::unique_ptr<cmph_t> cmph);

  std::unique_ptr<cmph_t> cmph_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_MPH_HPP_INCLUDED
