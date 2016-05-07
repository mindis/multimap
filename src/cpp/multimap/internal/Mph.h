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

#ifndef MULTIMAP_INTERNAL_MPH_H_
#define MULTIMAP_INTERNAL_MPH_H_

#include <memory>
#include <boost/filesystem/path.hpp>  // NOLINT
#include "multimap/thirdparty/cmph/cmph.h"
#include "multimap/Slice.h"

namespace multimap {
namespace internal {

class Mph {
  // This class is read-only and does not need external locking.

 public:
  Mph(Mph&&) = default;
  Mph& operator=(Mph&&) = default;

  static Mph build(const byte** keys, uint32_t nkeys);
  // A key in `keys` points to memory which is encoded like this:
  // [keylen as uint32][1 .. keydata .. keylen]

  static Mph build(const boost::filesystem::path& keys_file_path);
  // `keys` contains a sequence of keys which are encoded as described above.

  uint32_t operator()(const Slice& key) const;

  uint32_t size() const { return cmph_size(cmph_.get()); }

  static Mph readFromFile(const boost::filesystem::path& file_path);

  void writeToFile(const boost::filesystem::path& file_path) const;

 private:
  struct CmphDeleter {
    void operator()(cmph_t* cmph) const {
      if (cmph) cmph_destroy(cmph);
    }
  };

  explicit Mph(std::unique_ptr<cmph_t, CmphDeleter> cmph);

  std::unique_ptr<cmph_t, CmphDeleter> cmph_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_MPH_H_
