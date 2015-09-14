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

#ifndef MULTIMAP_INCLUDE_INTERNAL_AUTO_CLOSE_FILE_HPP
#define MULTIMAP_INCLUDE_INTERNAL_AUTO_CLOSE_FILE_HPP

#include <cassert>
#include <cstdio>

namespace multimap {
namespace internal {

class AutoCloseFile {
 public:
  AutoCloseFile() : file_(nullptr) {}

  explicit AutoCloseFile(std::FILE* file) : file_(file) {
    assert(file != nullptr);
  }

  ~AutoCloseFile() { reset(); }

  AutoCloseFile(AutoCloseFile&& other) : file_(other.file_) {
    other.file_ = nullptr;
  }

  AutoCloseFile& operator=(AutoCloseFile&& other) {
    if (&other != this) {
      reset(other.file_);
      other.file_ = nullptr;
    }
    return *this;
  }

  AutoCloseFile(const AutoCloseFile&) = delete;
  AutoCloseFile& operator=(const AutoCloseFile&) = delete;

  std::FILE* get() const { return file_; }

  void reset(std::FILE* file = nullptr) {
    if (file_) {
      const auto status = std::fclose(file_);
      assert(status == 0);
    }
    file_ = file;
  }

 private:
  std::FILE* file_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INCLUDE_INTERNAL_AUTO_CLOSE_FILE_HPP
