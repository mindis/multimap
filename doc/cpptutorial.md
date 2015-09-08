# C++ Tutorial

## Opening or Creating a Map

```cpp
#include <exception>
#include "multimap/Map.hpp"

multimap::Map map;
multimap::Options options;
options.block_size = 1024;  // Applies only if created.
options.create_if_missing = true;

try {
  map.Open("/path/to/directory", options);
} catch (std::exception& error) {
  HandleError(error);
}
```

If you want to throw an exception if the map already exists, you can set

```cpp
options.error_if_exists = true;
```

before calling `multimap::Map::Open`.

## Putting Values

Putting a value means to append a value to the end of the list that is
associated with a key. Both objects, the key and the value, must be of type
`Bytes` or be implicitly convertible to it.

```cpp
// Put a value constructed from a null-terminated C-string.
map.Put("key", "value");

// Sometimes std::string is used as a managed byte buffer.
const std::string value = Serialize(some_object);
map.Put("key", value);

// Or, the most general method, use a pointer to data plus its size.
// Note that PODs might cover more bytes than expected due to alignment.
map.Put("key", multimap::Bytes(&some_pod, sizeof some_pod));
```

## Getting Values

Getting values means to request an iterator to the list that is associated with
a key. The returned iterator can be mutable or immutable (const). The latter
does not allow to delete values while iterating. If for some key no such list
exists the iterator points to an empty list.

An iterator that points to a non-empty list also holds a lock on this list to
synchronize concurrent access. There are two types of locks, a reader and a
writer lock. A reader lock is acquired by an immutable iterator while a writer
lock is acquired by a mutable iterator. A list can be locked by multiple reader
locks, i.e. threads, at the same time while a writer lock requires exclusive
access. The lock is released when the iterator gets destroyed, normally when its
scope ends. However, iterators are movable and thus can be put into containers
or the like. When doing so you have to take special care not to run into
deadlocks.

```cpp
{
  multimap::Map::ConstIter iter = map.Get("key");
  // You might also use 'auto' here.

  for (iter.SeekToFirst(); iter.HasValue(); iter.Next()) {
    const multimap::Bytes value = iter.GetValue();
    // You might also use 'const auto' here.
    DoSomething(value);
  }
} // iter gets destroyed and the internal lock is released.
```

Iterators allow for lazy initialization which means that no IO operation is
performed until you call `SeekToFirst()` or one of its friends
`SeekTo(const Bytes&)` and `SeekTo(Predicate)`. This might be useful in cases
where you need to request multiple iterators first to determine in which order
they have to be processed depending on the lengths of the underlying lists. The
latter can be obtained via `NumValues()` which may be called even the iterator
has not been initialized.

A call to `GetValue()` returns the value the iterator currently points to. The
value itself is a shallow copy that points into memory managed by the iterator.
Hence, the value is only valid as long as the iterator is not moved forward. To
get a deep copy of the value you can call its `ToString()` method, use
[std::memcpy](http://en.cppreference.com/w/cpp/string/byte/memcpy) together
with its `data()` and `size()` methods, or parse the content into some rich
object using your preferred [serialization library](https://en.wikipedia.org/wiki/Comparison_of_data_serialization_formats).

## Deleting Values

## Replacing Values

## Closing a Map

