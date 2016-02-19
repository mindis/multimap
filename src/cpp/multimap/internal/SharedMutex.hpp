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

#ifndef MULTIMAP_INTERNAL_SHARED_MUTEX_HPP_INCLUDED
#define MULTIMAP_INTERNAL_SHARED_MUTEX_HPP_INCLUDED

#include <memory>
#include <boost/thread/shared_mutex.hpp>
#include "multimap/thirdparty/mt/mt.hpp"

namespace multimap {
namespace internal {

class SharedMutex : public mt::Resource {
  // This class serves the same purpose as boost::shared_mutex or
  // std::shared_mutex, but is designed for minimal memory footprint in order
  // to allow many simultaneous instances. In contrast to the mentioned mutexes
  // it allocates the actual mutex only on demand from a mutex pool or the free
  // store, and deallocates it when all locks have been released.

 public:
  SharedMutex() = default;

  void lock();

  bool try_lock();

  void unlock();

  void lock_shared();

  bool try_lock_shared();

  void unlock_shared();

 private:
  struct RefCountedMutex : public boost::shared_mutex {
    uint32_t refcount = 0;
  };

  static std::unique_ptr<RefCountedMutex> allocate();
  static void deallocate(std::unique_ptr<RefCountedMutex> mutex);

  std::unique_ptr<RefCountedMutex> mutex_;
};

static_assert(mt::hasExpectedSize<SharedMutex>(4, 8),
              "class SharedMutex does not have expected size");

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_SHARED_MUTEX_HPP_INCLUDED