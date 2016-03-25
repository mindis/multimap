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

namespace multimap {
namespace internal {

class SharedMutex {
  // This class serves the same purpose as std::shared_mutex (C++17) a.k.a.
  // boost::shared_mutex, but is designed for minimal memory footprint in order
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

  static size_t getCurrentPoolSize();
  static size_t getMaximumPoolSize();
  static void setMaximumPoolSize(size_t size);

 private:
  struct RefCountedMutex : public boost::shared_mutex {
    uint32_t refcount = 0;
  };

  struct Pool {
    static Pool& instance();
    size_t getCurrentSize() const;
    size_t getMaximumSize() const;
    void setMaximumSize(size_t size);
    void release(std::unique_ptr<RefCountedMutex> mutex);
    std::unique_ptr<RefCountedMutex> acquire();

   private:
    Pool() = default;
    size_t max_size_ = 1024;
    std::vector<std::unique_ptr<RefCountedMutex>> mutexes_;
  };

  static std::unique_ptr<RefCountedMutex> allocate();
  static void deallocate(std::unique_ptr<RefCountedMutex> mutex);

  std::unique_ptr<RefCountedMutex> mutex_;
};

}  // namespace internal
}  // namespace multimap

#endif  // MULTIMAP_INTERNAL_SHARED_MUTEX_HPP_INCLUDED
