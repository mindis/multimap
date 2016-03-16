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

#include "multimap/internal/Store.hpp"

#include <cstdio>
#include <boost/filesystem/operations.hpp>

#include "multimap/thirdparty/mt/mt.hpp"

namespace multimap {
namespace internal {

Store::Store(const boost::filesystem::path& filename, const Options& options)
    : options_(options) {
  MT_REQUIRE_NOT_ZERO(options.block_size);
  if (boost::filesystem::is_regular_file(filename)) {
    fd_ = mt::open(filename, options.readonly ? O_RDONLY : O_RDWR);
    const uint64_t file_size = mt::seek(fd_.get(), 0, SEEK_END);
    mt::Check::isZero(file_size % options.block_size,
                      "Store: block size does not match size of data file");
    //    if (file_size != 0) {
    //      auto prot = PROT_READ;
    //      if (!options.readonly) {
    //        prot |= PROT_WRITE;
    //      }
    //      mapped_.data =
    //          mt::mmap(file_size, prot, MAP_SHARED, ::fileno(stream_.get()),
    //          0);
    //      mapped_.size = file_size;
    //    }

  } else {
    fd_ = mt::open(filename, O_RDWR | O_CREAT, 0644);
  }
}

}  // namespace internal
}  // namespace multimap
