## Opening a Map

An already existing map can be opened in two ways:

1. Calling the constructor `multimap::Map(directory, options)`
2. Calling the default constructor `multimap::Map()` followed by `multimap::Map::Open(directory, options)`

A new map can be created setting `options.create_if_missing` to `true`.

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

If you want an exception to be thrown if the map already exists, set `options.error_if_exists` to `true` before calling `multimap::Map::Open`.

## Putting Values

Putting a value means to append a value to the end of the list that is associated with a key. Both objects, the key and the value, must be of type `Bytes` or be implicitly convertible to it.

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

Getting values means to request an iterator to the list that is associated with a key. The returned iterator can be mutable or immutable (const). The latter does not allow to delete values while iterating. If for some key no such list exists the iterator points to an empty list.

An iterator that points to a non-empty list also holds a lock on this list to synchronize concurrent access. There are two types of locks, a reader and a writer lock. A reader lock is acquired by an immutable iterator while a writer lock is acquired by a mutable iterator. A list can be locked by multiple reader locks, i.e. threads, at the same time while a writer lock requires exclusive access. The lock is released when the iterator gets destroyed, normally when its scope ends. However, iterators are movable and thus can be put into containers or the like. When doing so you have to take special care not to run into deadlocks.

```cpp
{
  multimap::Map::ConstIter iter = map.Get("key");
  // You could also use 'auto' here.

  for (iter.SeekToFirst(); iter.HasValue(); iter.Next()) {
    const multimap::Bytes value = iter.GetValue();
    // You might also use 'const auto' here.
    DoSomething(value);
  }
} // iter gets destroyed and the internal lock is released.
```

Iterators allow for lazy initialization which means that no IO operation is performed until you call `SeekToFirst()` or one of its friends `SeekTo(target)` and `SeekTo(predicate)`. This might be useful in cases where you need to request multiple iterators first to determine in which order they have to be processed depending on the lengths of the underlying lists. The latter can be obtained via `NumValues()` which may be called even the iterator has not been initialized.

A call to `GetValue()` returns the value the iterator currently points to. The value itself is a shallow copy that points into memory managed by the iterator. Hence, the value is only valid as long as the iterator is not moved forward. To get a deep copy of the value you can call its `ToString()` method, use [std::memcpy](http://en.cppreference.com/w/cpp/string/byte/memcpy) together with its `data()` and `size()` methods, or parse the content into some rich object using your preferred [serialization library](https://en.wikipedia.org/wiki/Comparison_of_data_serialization_formats).

## Deleting Values

Values can be deleted using a mutable iterator.

```cpp
multimap::Map::Iter iter = map.GetMutable("key");
// You could also use 'auto' here.

for (iter.SeekToFirst(); iter.HasValue(); iter.Next()) {
  if (CanBeDeleted(iter.GetValue())) {
    iter.DeleteValue();
    // iter.HasValue() is false now.
    // iter.Next() will seek to the next value, if any.
  }
}
```

Alternatively, you can use one of the methods provided by class `multimap::Map`.

* [`multimap::Map::Delete(key)`](cppreference.md#mapdelete)
* [`multimap::Map::DeleteFirst(key, predicate)`](cppreference.md#mapdeletefirst)
* [`multimap::Map::DeleteFirstEqual(key, value)`](cppreference.md#mapdeletefirstequal)
* [`multimap::Map::DeleteAll(key, predicate)`](cppreference.md#mapdeleteall)
* [`multimap::Map::DeleteAllEqual(key, value)`](cppreference.md#mapdeleteallequal)

When a value is deleted it will initially marked as deleted so that it will be ignored by any subsequent operation. Only the functions [`multimap::Optimize`](cppreference.md#optimize) or [`multimap::Export`](cppreference.md#export) will remove the data physically.

## Replacing Values

## Closing a Map

A map is closed automatically when its lifetime ends and its destructor is called. There is no explicit `Close` method. If you really need to transfer ownership of a map you can allocate it on the heap via `new` and let the result be managed by a smart pointer. If you use a raw pointer and forget to call `delete` data is definitely lost. A map object itself is neither copyable nor movable. 
