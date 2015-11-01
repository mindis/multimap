## Opening a Map

An already existing map can be opened in two ways:

1. Calling the constructor `multimap::Map(directory, options)`
2. Calling the default constructor `multimap::Map()` followed by `multimap::Map::Open(directory, options)`

A new map can be created setting `options.create_if_missing` to `true`.

```cpp
#include <exception>
#include <boost/filesystem/path.hpp>
#include <multimap/Map.hpp>

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

## Closing a Map

A map is closed automatically when its lifetime ends and its destructor is called. There is no explicit `Close()` method. If you really need to transfer ownership of a map allocate it via `new` and let the result be managed by a smart pointer. If you use a raw pointer and forget to call `delete` data is definitely lost. A map object itself is neither copyable nor movable.

## Putting Values

Putting a value means to append a value to the end of the list that is associated with a key. Both objects, the key and the value, must be of type `multimap::Bytes` or be implicitly convertible to it.

```cpp
const multimap::Bytes key = "key";

// Put a value constructed from a null-terminated C-string.
map.Put(key, "value");

// Put a value constructed from a pointer to data plus its size.
// Note that PODs might cover more bytes than expected due to alignment.
map.Put(key, multimap::Bytes(&some_pod, sizeof some_pod));

// Put a value constructed from std::string sometimes used as a byte buffer.
const std::string value = somelib::Serialize(some_object);
map.Put(key, value);
```

## Getting Values

Getting values means to request an iterator to the list that is associated with a key. The returned iterator can be mutable or immutable (const). The latter does not allow to delete values while iterating. If for some key no such list exists the iterator points to an empty list.

An iterator that points to a non-empty list also holds a lock on this list to synchronize concurrent access. There are two types of locks, a reader and a writer lock. A reader lock is acquired by an immutable iterator while a writer lock is acquired by a mutable iterator. A list can be locked by multiple reader locks, i.e. threads, at the same time while a writer lock requires exclusive access. The lock is released when the iterator gets destroyed, normally when its scope ends. However, iterators are movable and thus can be put into containers or the like. When doing so you have to take special care not to run into deadlocks.

```cpp
{
  multimap::Map::ConstIter iter = map.Get(key);
  // You could also use 'auto' here.

  for (iter.SeekToFirst(); iter.HasValue(); iter.Next()) {
    const multimap::Bytes value = iter.GetValue();
    // You might also use 'const auto' here.
    DoSomething(value);
  }
} // iter gets destroyed and the internal lock is released.
```

Iterators allow for lazy initialization which means that no IO operation is performed until you call `SeekToFirst()` or one of its friends `SeekTo(target)` and `SeekTo(predicate)`. This might be useful in cases where you need to request multiple iterators first to determine in which order they have to be processed. The length of the underlying list can be obtained via `NumValues()` even if the iterator has not been initialized.

A call to `GetValue()` returns the value the iterator currently points to. The value itself is a shallow copy that points into memory managed by the iterator. Hence, the value is only valid as long as the iterator is not moved forward. To get a deep copy of the value you can call its `ToString()` method, use `std::memcpy` together with its `data()` and `size()` methods, or parse the content into some rich object using your preferred [serialization library](https://en.wikipedia.org/wiki/Comparison_of_data_serialization_formats).

## Deleting Values

When a value is deleted it will initially be marked as deleted so that it will be ignored by any subsequent operation. Only the functions `multimap::Optimize` or `multimap::Export` will remove the data physically.

Values can be deleted using a mutable iterator:

```cpp
multimap::Map::Iter iter = map.GetMutable(key);
// You could also use 'auto' here.

for (iter.SeekToFirst(); iter.HasValue(); iter.Next()) {
  if (CanBeDeleted(iter.GetValue())) {
    iter.DeleteValue();
    // iter.HasValue() is false now.
    // iter.Next() will seek to the next value, if any.
  }
}
```

or using the following methods provided by class `multimap::Map`:

* `std::size_t Delete(key)`
* `std::size_t DeleteAll(key, predicate)`
* `std::size_t DeleteAllEqual(key, value)`
* `bool DeleteFirst(key, predicate)`
* `bool DeleteFirstEqual(key, value)`

Example 1: Delete every value (of type `std::uint32_t`) that is even.

```cpp
// Signature must match multimap::Callables::Predicate.
const auto is_even = [](const multimap::Bytes& value) {
  std::uint32_t number;
  std::memcpy(&number, value.data(), value.size());
  return number % 2 == 0;
};

// Before: key -> (0, 1, 2, 3, 4, 5, 6, 7, 8, 9)
const std::size_t num_deleted = map.DeleteAll(key, is_even);
// After: key -> (1, 3, 5, 7, 9)
// num_deleted == 5
```

Example 2: Delete every value (of type `std::uint32_t`) that is equal to `23`.

```cpp
const std::uint32_t number = 23;
const multimap::Bytes value(&number, sizeof number);

// Before: key -> (0, 1, 23, 45, 67, 89, 23)
const std::size_t num_deleted = map.DeleteAllEqual(key, value);
// After: key -> (0, 1, 45, 67, 89)
// num_deleted == 2
```

## Replacing Values

When a value is replaced the old value is marked as deleted and the new value is appended to the end of the list. Hence, replacing operations will not preserve the order of values. There is an open [issue](https://bitbucket.org/mtrenkmann/multimap/issues/2) to support in-place replacements in future releases of library.

Values can be replaced using the following methods provided by class `multimap::Map`:

* `std::size_t ReplaceAll(key, function)`
* `std::size_t ReplaceAllEqual(key, old_value, new_value)`
* `bool ReplaceFirst(key, function)`
* `bool ReplaceFirstEqual(key, old_value, new_value)`

Example 1: Replace every value (of type `std::uint32_t`) that is even by its natural successor.

```cpp
// Signature must match multimap::Callables::Function.
const auto increment_if_even = [](const multimap::Bytes& value) {
  std::uint32_t number;
  std::memcpy(&number, value.data(), value.size());
  std::string result;
  if (number % 2 == 0) {
    ++number;
    char buf[sizeof number];
    std::memcpy(buf, &number, sizeof number);
    result.assign(buf, sizeof number);
  }
  return result;
};

// Before: key -> (0, 1, 2, 3, 4, 5, 6, 7, 8, 9)
const std::size_t num_replaced = map.ReplaceAll(key, increment_if_even);
// After: key -> (1, 3, 5, 7, 9, 1, 3, 5, 7, 9)
// num_replaced == 5
```

Example 2: Replace every value (of type `std::uint32_t`) that is equal to `23` by `42`.

```cpp
const std::uint32_t old_number = 23;
const std::uint32_t new_number = 42;
const multimap::Bytes old_value(&old_number, sizeof old_number);
const multimap::Bytes new_value(&new_number, sizeof new_number);

// Before: key -> (0, 1, 23, 45, 67, 89, 23)
const std::size_t num_replaced = map.ReplaceAllEqual(key, old_value, new_value);
// After: key -> (0, 1, 45, 67, 89, 42, 42)
// num_replaced == 2
```
