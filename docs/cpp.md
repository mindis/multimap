```{cpp}
#include "multimap/Map.hpp"
```

## Map::Open

`static std::unique_ptr<Map> Open(const std::string& directory, const Options& options)`

Opens a map or creates a new one from/in `directory`.

Throws: `std::runtime_error` if something went wrong.

```{cpp}
multimap::Options options;
options.block_size = 512;
options.block_pool_memory = multimap::GiB(2);
options.create_if_missing = true;

auto map = multimap::Map::Open("/path/to/multimap", options);
```

## Map::Put

`void Put(const Bytes& key, const Bytes& value)`

Puts `value` into the list associated with `key`.

Throws: `std::runtime_error` if the value was too big. See [Limitations](index.md#limitations) for more details.

```{cpp}
map->Put("key", "value");
map->Put("key", std::to_string(12));

const auto pair = std::make_pair(34, 5.67);
map->Put("key", multimap::Bytes(&pair, sizeof pair));
// sizeof pair might be 16 due to alignment.
```

## Map::Get

`ConstIter Get(const Bytes& key) const`

Tries to acquire a read lock on the list associated with `key` and, if successful, returns a read-only iterator on it. If another thread already holds a write lock on the same list, the method blocks until the write lock is released. Multiple threads may acquire a read lock on the same list at the same time.

```{cpp}
auto iter = map->Get("key");
for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
  const multimap::Bytes value = iter.Value();
  DoSomething(value);
  // The value object actually points into an internal buffer
  // that may change during the iteration.  Therefore, if you
  // want to keep a copy of the value you can do so via
  // std::memcpy(dest_buf, value.data(), value.size())
  // or value.ToString().
}
```

## Map::GetMutable

`Iter GetMutable(const Bytes& key)`

Tries to acquire a write lock on the list associated with `key` and, if successful, returns an iterator that implements a `Delete` operation. If another thread already holds a read or write lock on the same list, the method blocks until all locks are released.

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

## Map::Contains

`bool Contains(const Bytes& key) const`

Checks if there is a list associated with `key`. Note that [Map::Get](#mapget) or [Map::GetMutable](#mapgetmutable) cannot be used for a containment check, because they always return an iterator of zero size if the underlying list is empty or even does not exist.


```{cpp}
const bool exists = map->Contains("key");
```

## Map::Delete

`std::size_t Delete(const Bytes& key)`

Deletes the entire list associated with `key`. If the list is currently locked, the method blocks until all locks are released.

Returns: The size of the deleted list.

```{cpp}
const std::size_t size = map->Delete("key");
```

## Map::DeleteFirst

`bool DeleteFirst(const Bytes& key, ValuePredicate predicate)`

Deletes the first value in the list associated with `key` for which `predicate` returns `true`.

Returns: `true` if a value was deleted, `false` otherwise.

```{cpp}
const auto predicate = [](const multimap::Bytes& value) {
  bool can_be_deleted = false;
  // Check value and set can_be_deleted = true to delete it.
  return can_be_deleted;
};
const bool success = map->DeleteFirst("key", predicate);
```

## Map::DeleteFirstEqual

`bool DeleteFirstEqual(const Bytes& key, const Bytes& value)`

Deletes all values in the list associated with `key` that are equal to `value` according to `operator==`.

Returns: `true` if a value was deleted, `false` otherwise.

```{cpp}
const bool success = map->DeleteFirstEqual("key", "pattern");
```

## Map::DeleteAll

`std::size_t DeleteAll(const Bytes& key, ValuePredicate predicate)`

Deletes all values in the list associated with `key` for which `predicate` returns `true`.

Returns: The number of values deleted.

```{cpp}
const auto predicate = [](const multimap::Bytes& value) {
  bool can_be_deleted = false;
  // Check value and set can_be_deleted = true to delete it.
  return can_be_deleted;
};
const auto num_deleted = map->DeleteAll("key", predicate);
```

## Map::DeleteAllEqual

`std::size_t DeleteAllEqual(const Bytes& key, const Bytes& value)`

Deletes all values in the list associated with `key` that are equal to `value` according to `operator==`.

Returns: The number of values deleted.

```{cpp}
const auto num_deleted = map->DeleteAllEqual("key", "pattern");
```

## Map::ReplaceFirst

`bool ReplaceFirst(const Bytes& key, ValueFunction function)`

Replaces the first value in the list associated with `key` by the result of `function`, if any. The replacement does not happen in-place. Instead, the old value is marked as deleted and the new value is appended to the end of the list. There is an [issue](https://bitbucket.org/mtrenkmann/multimap/issues/2/in-place-map-replace) to allow in-place replacements.

Returns: `true` if a value was replaced, `false` otherwise.

```{cpp}
const auto replace = [](const multimap::Bytes& value) {
  std::string new_value;
  // Fill new_value with content or leave it empty.
  return new_value;
};
const auto success = map->ReplaceFirst("key", replace);
```

## Map::ReplaceFirstEqual

`bool ReplaceFirstEqual(const Bytes& key, const Bytes& old_value, const Bytes& new_value)`

Replaces the first occurrence of `old_value` in the list associated with `key` by `new_value`. The replacement does not happen in-place. Instead, the old value is marked as deleted and the new value is appended to the end of the list. There is an [issue](https://bitbucket.org/mtrenkmann/multimap/issues/2/in-place-map-replace) to allow in-place replacements.

Returns: `true` if a value was replaced, `false` otherwise.

```{cpp}
const auto success = map->ReplaceFirstEqual("key", "old value", "new value");
```

## Map::ReplaceAll

`std::size_t ReplaceAll(const Bytes& key, ValueFunction function)`

Replaces all values in the list associated with `key` by the result of `function`, if any. The replacement does not happen in-place. Instead, the old value is marked as deleted and the new value is appended to the end of the list. There is an [issue](https://bitbucket.org/mtrenkmann/multimap/issues/2/in-place-map-replace) to allow in-place replacements.

Returns: The number of replaced values.

```{cpp}
const auto replace = [](const multimap::Bytes& value) {
  std::string new_value;
  // Fill new_value with content or leave it empty.
  return new_value;
};
const auto num_replaced = map->ReplaceAll("key", replace);
```

## Map::ReplaceAllEqual

`std::size_t ReplaceAllEqual(const Bytes& key, const Bytes& old_value, const Bytes& new_value)`

Replaces all occurrences of `old_value` in the list associated with `key` by `new_value`. The replacements do not happen in-place. Instead, the old value is marked as deleted and the new value is appended to the end of the list. There is an [issue](https://bitbucket.org/mtrenkmann/multimap/issues/2/in-place-map-replace) to allow in-place replacements.

Returns: The number of replaced values.

```{cpp}
const auto num_replaced = map->ReplaceAllEqual("key", "old value", "new value");
```

## Map::ForEachKey

## Map::GetProperties

## Map::PrintProperties

## Map::ValuePredicate

## Map::ValueFunction

## Map::ConstIter

An iterator releases its lock automatically when it gets destroyed. An iterator cannot be copied for single ownership reasons, but moving is allowed.

Apart from that, the iterator

- must be initialized via `SeekToFirst` before iteration.
- has a `Size` method that returns the number of values, even if not yet initialized.
- has a size of zero, if no mapping for the given key exists.

## Map::Iter

Has exclusive ownership.
