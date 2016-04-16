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

#ifndef MULTIMAP_INTERNAL_TSV_FILE_WRITER_HPP_INCLUDED
#define MULTIMAP_INTERNAL_TSV_FILE_WRITER_HPP_INCLUDED

#include <boost/filesystem/fstream.hpp>
#include "multimap/Iterator.hpp"
#include "multimap/Slice.hpp"

namespace multimap {
namespace internal {

class TsvFileWriter {
 public:
  explicit TsvFileWriter(const boost::filesystem::path& file_path);

  void write(const Slice& key, const Slice& value);

  void write(const Slice& key, Iterator* iter);

 private:
  boost::filesystem::ofstream stream_;
  std::string base64_key_;
  std::string base64_value_;
  Bytes current_key_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_TSV_FILE_WRITER_HPP_INCLUDED
