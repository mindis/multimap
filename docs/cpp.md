```{cpp}
#include "multimap/Map.hpp"
```

## Open

```{cpp}
multimap::Options options;
options.block_size = 512;
options.block_pool_memory = multimap::GiB(2);
options.create_if_missing = true;

auto map = multimap::Map::Open("/path/to/multimap", options);
```

## Put

Keys and values are of type `Bytes`, which is a raw memory wrapper. A `Bytes` object can be constructed from C-style strings, STL strings, or data/size pairs.

```{cpp}
map->Put("key", "value");
map->Put("key", std::to_string(42));

const double pi = 3.14;
map->Put("key", multimap::Bytes(&pi, sizeof pi));
```

## Get

Tries to acquire a read lock on the associated list of values and returns a const iterator for read-only operations. If another thread has already acquired a write lock on the same list, the method blocks until the write lock is released.

Multiple threads can hold a read lock on the same list at the same time. An iterator releases its lock automatically when it gets destroyed. You cannot copy an iterator for unique ownership reasons, but moving is allowed.

Apart from that, an iterator

- must be initialized via `SeekToFirst` before iteration.
- has a `Size` method that returns the number of values, even if not yet initialized.
- has a size of zero, if no mapping for the given key exists.

```{cpp}
auto iter = map->Get("key");
if (iter.Size() != 0) {
  // The size check is optional.
  for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
    const multimap::Bytes value = iter.Value();
    DoSomething(value);
    // The value object actually points into an internal buffer
    // that may change during the iteration.  Therefore, if you
    // want to keep a copy of the value you can do so via
    // std::memcpy(dest_buf, value.data(), value.size())
    // or value.ToString().
  }
}
```

## GetMutable

## Contains

## Delete

## DeleteFirst

## DeleteFirstEqual

## DeleteAll

## DeleteAllEqual

## ReplaceFirst

## ReplaceAll

## UpdateFirst

## UpdateAll

## ForEachKey

## GetProperties

## PrintProperties
