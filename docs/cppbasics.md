<br>

```cpp
#include <multimap/Map.hpp>
```

## Creating a Map

The minimal code to create a map is

```cpp
multimap::Options options;
options.create_if_missing = true;
multimap::Map map("path/to/directory", options);
```

However, since a Map object is neither copyable nor moveable you might allocate it on the heap to be able to transfer ownership. In case of an error the constructor of Map also throws an exception that should be caught. So you would better write code like this:

```cpp
std::unique_ptr<multimap::Map> map;
try {
  multimap::Options options;
  options.create_if_missing = true;
  map.reset(new Map("path/to/directory", options));
} catch (std::runtime_error& error) {
  handleError(error);
}
```

## Opening a Map

To open an already existing map with default options you can simply omit the options parameter.

```cpp
std::unique_ptr<multimap::Map> map;
try {
  map.reset(new Map("path/to/directory"));
} catch (std::runtime_error& error) {
  handleError(error);
}
```

If the map does not exist, an exception will be thrown.

## Closing a Map

A map is closed automatically when its destructor is called. There is no explicit close method. If you really want to force destruction you can reset the smart pointer.

```cpp
map.reset();
```

## Putting Values

Putting a value means to append a value to the end of the list that is associated with a key. Both the key and the value are of type [Bytes](cppreference#class-bytes) which can be constructed implicitly from

* a null-terminated `const char*`,
* a `std::string`,

or explicitly by a `const void*` to raw data and its size.

```cpp
const multimap::Bytes key = "key";

map->put(key, "value");
// Puts a value implicitly constructed from a null-terminated C-string.

map->put(key, std::to_string(23));
// Puts a value implicitly constructed from a standard string.

map->put(key, multimap::Bytes(&some_pod, sizeof some_pod));
// Puts a value explicitly constructed from a pointer to data and its size.
// Note that PODs might cover more bytes than expected due to alignment.
```

It is important to mention that a Bytes object is always just a pointer/size wrapper without any ownership semantics. The actual lifetime of the wrapped data must be managed by the user and must outlive the lifetime of the Bytes object.

## Getting Values

Getting values means to request a read-only [Iterator](cppreference#type-mapiterator) to the list associated with a key. If no such list exists the returned iterator points to an empty list. An iterator that points to a non-empty list holds a [reader lock](cppreference#reader-lock) on this list to synchronize concurrent access. A list can be read by multiple iterators from different threads at the same time. The lock is released automatically when the iterator's lifetime ends.

```cpp
{
  multimap::Map::Iterator iter = map->get(key);
  // You could also have used 'auto' here.

  while (iter.hasNext()) {
    const multimap::Bytes value = iter.next();
    // You could also have used 'auto' here.
    // `value` points to data managed by the iterator,
    // and which is valid until the next call to `next`.
    
    doSomething(value);
    
    std::cout << value.toString() << '\n';
    // Printing makes sense only if `value`
    // actually contains printable characters.
  }
} // iter gets destroyed and the internal reader lock is released.
```

Things about iterators that are also worth noting:

* Iterators are movable and thus can be put into containers or the like. When doing so you have to take special care not to run into deadlocks, because
* there is another type of iterator which holds a writer lock on a list for exclusive access. This type of iterator is only used internally for implementing remove and replace operations.
* Iterators support lazy initialization which means that no I/O operation is performed until its `next()` method has been called. This feature is useful in cases where multiple iterators must be requested first in order to determine in which order they have to be processed.
* Calling `next()` followed by `toString()` is a simple way to get a deep copy of the current value.
* Iterators can tell the number of values left to iterate by invoking `available()`.

## Removing Values

Values can be removed from a list by injecting a [Predicate](cppreference#predicate) which yields true for values to be removed. There are methods to remove only the first or all matches from a list. Since this is a mutating operation the methods require exclusive access to the list and therefore try to acquire a [writer lock](cppreference#writer-lock) for that list.

```cpp
#include <multimap/callables.hpp>

map->put(key, "1");
map->put(key, "2");
map->put(key, "3");
map->put(key, "4");
map->put(key, "5");
map->put(key, "6");

map->removeValue(key, multimap::Equal("3"));
// Removes the first value equal to "3".
// key -> {"1", "2", "4", "5", "6"}

const auto is_even = [](const multimap::Bytes& value) {
  return std::stoi(value.toString()) % 2 == 0;
}

map->removeValue(key, is_even);
// Removes the first value that is even.
// key -> {"1", "4", "5", "6"}

map->removeValues(key, is_even);
// Removes all values that are even.
// key -> {"1", "5"}

map->removeKey(key);
// Removes all values.
// key -> {}
```

When a value is removed it will only be marked as removed for the moment so that subsequent iterations will ignore it. Only an optimize operation triggered either via [API call](cppreference#map-optimize) or the [command line tool](overview#multimap-optimize) will remove the data physically.

Note that if the list is already locked either by a [reader](cppreference#reader-lock) or [writer lock](cppreference#writer-lock) any remove operation on that list will block until the lock is released. A thread should never try to remove values while holding iterators to the same list to avoid deadlocks.

## Replacing Values

Values can be replaced by injecting a [Procedure](cppreference#procedure) that represents a map function which yields a non-empty string for values to be replaced. There are methods to replace only the first or all matches in a list. Since this is a mutating operation the methods require exclusive access to the list and therefore try to acquire a [writer lock](cppreference#writer-lock) for that list.

When a value is replaced the old value is marked as removed and the new value is appended to the end of the list. Hence, replace operations will not preserve the order of values. If a certain order needs to be restored an [optimize](cppreference#map-optimize) operation has to be run parameterized with an appropriate [Compare](cppreference#compare) function.

```cpp
#include <multimap/callables.hpp>

map->put(key, "1");
map->put(key, "2");
map->put(key, "3");
map->put(key, "4");
map->put(key, "5");
map->put(key, "6");

map->replaceValue(key, "3", "4");
// Replaces the first value equal to "3" by "4".
// key -> {"1", "2", "4", "5", "6", "4"}

const auto increment_if_even = [](const multimap::Bytes& value) {
  const auto number = std::stoi(value.toString());
  return (number % 2 == 0) ? std::to_string(number + 1) : "";
};

map->replaceValue(key, increment_if_even);
// Replaces the first value that is even by its successor.
// key -> {"1", "4", "5", "6", "4", "3"}

map->replaceValues(key, "4", "8");
// Replaces all values equal to "4" by "8".
// key -> {"1", "5", "6", "3", "8", "8"}

map->replaceValues(key, increment_if_even);
// Replaces all values that are even by its successor.
// key -> {"1", "5", "3", "7", "9", "9"}
```

Note that if the list is already locked either by a [reader](cppreference#reader-lock) or [writer lock](cppreference#writer-lock) any replace method will block until the lock is released. A thread should never try to replace values while holding iterators to the same list to avoid deadlocks.
