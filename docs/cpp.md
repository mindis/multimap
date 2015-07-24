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

Keys and values are of type `Bytes`, which is a non-owning raw memory wrapper. A `Bytes` object can be constructed from C-style strings, STL strings, or pointer/size pairs. The maximum size of a key or value is 2^15-1 (= 32767) bytes.

```{cpp}
map->Put("key", "value");
map->Put("key", std::to_string(12));

const auto pair = std::make_pair(34, 5.67);
map->Put("key", multimap::Bytes(&pair, sizeof pair));
// sizeof pair might be 16 due to alignment.
```

## Get

Tries to acquire a read lock on the associated list of values and, if successful, returns a `Map::ConstIter` for read-only iteration. If another thread has already acquired a write lock on the same list, the method blocks until the write lock is released.

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

Tries to acquire a write lock on the associated list of values and, if successful, returns a `Map::Iter` that implements a `Delete` operation. If another thread has already acquired a read or write lock on the same list, the method blocks until all locks are released. In other words, this iterator requires exclusive access to the underlying list. The usage is otherwise identical to `Map::ConstIter`. See [Get](#get) for more information.

```{cpp}
auto iter = map->GetMutable("key");
for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
  if (CanBeDeleted(iter.Value())) {
    iter.Delete();
    // The value is now marked as deleted.  The operation is not reversible.
    // iter.Valid() would yield false, so you must not call iter.Value() again.
    // iter.Next() will advance the iterator to the next valid value.
  }
}
```

## Contains

Checks if a list for a given key exists. This operation is a bit cheaper than calling [Get](#get) or [GetMutable](#getmutable).


```{cpp}
const bool exists = map->Contains("key");
```

## Delete

Deletes the entire list associated with a given key. If another thread holds a lock on the list, the method blocks until all locks are released. Returns the size of the deleted list, which may be zero if the list was empty or no such list did exist.

```{cpp}
const std::size_t size = map->Delete("key");
```

## DeleteFirst

Deletes the first value in the associated list that matches a given predicate. Returns `true` on success and `false` otherwise. The latter/worst case requires a complete list scan. The predicate can be any callable that can be converted or bound to `multimap::Map::ValuePredicate`.

```{cpp}
// multimap::Map::ValuePredicate is defined as
// typedef std::function<bool(const Bytes&)> ValuePredicate;

const auto predicate = [](const multimap::Bytes& value) {
  return CanBeDeleted(value);
};
const bool success = map->DeleteFirst("key", predicate);
```

## DeleteFirstEqual

Same as [DeleteFirst](#deletefirst) but uses `operator==` on type `Bytes` together with some user-supplied value as the predicate function. In other words, it deletes the first value in the associated list that is equal to the supplied value.

```{cpp}
const bool success = map->DeleteFirstEqual("key", "pattern");
// Note that "pattern" is implicitly convertible to multimap::Bytes.
```

## DeleteAll

## DeleteAllEqual

## ReplaceFirst

## ReplaceAll

## UpdateFirst

## UpdateAll

## ForEachKey

## GetProperties

## PrintProperties
