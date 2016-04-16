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

#ifndef MULTIMAP_INTERNAL_LOCKS_HPP_INCLUDED
#define MULTIMAP_INTERNAL_LOCKS_HPP_INCLUDED

#include <boost/thread/lock_guard.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/shared_lock_guard.hpp>
#include "multimap/thirdparty/mt/fileio.hpp"
#include "multimap/internal/Descriptor.hpp"

namespace multimap {
namespace internal {

template <typename SharedMutex>
using ReaderLock = boost::shared_lock<SharedMutex>;

template <typename SharedMutex>
using ReaderLockGuard = boost::shared_lock_guard<SharedMutex>;

template <typename SharedMutex>
using WriterLock = boost::unique_lock<SharedMutex>;

template <typename SharedMutex>
using WriterLockGuard = boost::lock_guard<SharedMutex>;

constexpr boost::try_to_lock_t TRY_TO_LOCK{};

struct DirectoryLock {
 public:
  DirectoryLock(const boost::filesystem::path& directory)
      : dlock_(directory, Descriptor::getFilePrefix() + ".lock") {}

  const boost::filesystem::path& directory() const {
    return dlock_.directory();
  }

  const std::string& file_name() const { return dlock_.file_name(); }

 private:
  mt::DirectoryLockGuard dlock_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_LOCKS_HPP_INCLUDED
