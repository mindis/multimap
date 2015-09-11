## `class Bytes`

```cpp
#include <multimap/Bytes.hpp>
namespace multimap
```

This class represents a wrapper for raw byte data without ownership management.

Members       |
-------------:|-----------------------------------------------------------------
              | [`Bytes()`](#bytes-bytes)
              | [`Bytes(const char* cstr)`](#bytes-bytes-cstr)
              | [`Bytes(const std::string& str)`](#bytes-bytes-str)
              | [`Bytes(const void* data, std::size_t size)`](#bytes-bytes-data-size)
`const char*` | [`data() const`](#bytes-data)
`std::size_t` | [`size() const`](#bytes-size)
`bool`        | [`empty() const`](#bytes-empty)
`void`        | [`clear()`](#bytes-clear)
`std::string` | [`ToString() const`](#bytes-tostring)

Functions     |
-------------:|-----------------------------------------------------------------
`inline bool` | [`operator==(const Bytes& lhs, const Bytes& rhs)`](#operator-eq)
`inline bool` | [`operator!=(const Bytes& lhs, const Bytes& rhs)`](#operator-ne)
`inline bool` | [`operator<(const Bytes& lhs, const Bytes& rhs)`](#operator-lt)

<span class='declaration' id='bytes-bytes'>`Bytes()`</span>

Creates an instance that refers to an empty array.

Postconditions:

* `data() != nullptr`
* `size() == 0`

<span class='declaration' id='bytes-bytes-cstr'>`Bytes(const char* cstr)`</span>

Creates an instance that wraps a null-terminated C-string.

Preconditions:

* `cstr != nullptr`

Postconditions:

* `data() == cstr`
* `size() == std::strlen(cstr)`

<span class='declaration' id='bytes-bytes-str'>`Bytes(const std::string& str)`</span>

Creates an instance that wraps a standard string.

Postconditions:

* `data() == str.data()`
* `size() == str.size()`

<span class='declaration' id='bytes-bytes-data-size'>`Bytes(const void* data, std::size_t size)`</span>

Creates an instance that wraps a pointer to data of `size` bytes.

Preconditions:

* `data != nullptr`

Postconditions:

* `data() == data`
* `size() == size`

<span class='declaration' id='bytes-data'>`const char* data() const`</span>

Returns a read-only pointer to the wrapped data.

<span class='declaration' id='bytes-size'>`std::size_t size() const`</span>

Returns the number of bytes wrapped.

<span class='declaration' id='bytes-empty'>`bool empty() const`</span>

Returns `true` if the number of bytes wrapped is zero, and `false` otherwise.

<span class='declaration' id='bytes-clear'>`void clear()`</span>

Let this instance refer to an empty array.

Postconditions:

* `data() != nullptr`
* `size() == 0`

<span class='declaration' id='bytes-tostring'>`std::string ToString() const`</span>

Returns a deep copy of the wrapped data. `std::string` is used here as a convenient byte buffer which may contain characters that are not printable.

Postconditions:

* `data() != result.data()`
* `size() == result.size()`

<span class='declaration' id='operator-eq'>`inline bool operator==(const Bytes& lhs, const Bytes& rhs)`</span>

Returns `true` if `lhs` and `rhs` contain the same number of bytes and which are equal after byte-wise comparison. Returns `false` otherwise.

<span class='declaration' id='operator-ne'>`inline bool operator!=(const Bytes& lhs, const Bytes& rhs)`</span>

Returns `true` if the bytes wrapped by `lhs` and `rhs` are not equal after byte-wise comparison. Returns `false` otherwise.

<span class='declaration' id='operator-lt'>`inline bool operator<(const Bytes& lhs, const Bytes& rhs)`</span>

Returns `true` if `lhs` is less than `rhs` according to `std::memcmp`, and `false` otherwise. If `lhs` and `rhs` do not wrap the same number of bytes, only the first `std::min(lhs.size(), rhs.size())` bytes will be compared.

## `class Callables`

```cpp
#include <multimap/Callables.hpp>
namespace multimap
```

This class contains typedefs which define signatures of various callable types.

Members   |
---------:|---------------------------------------------------------------------
`typedef` | [`std::function<void(const Bytes&)> Procedure`](#callables-procedure)
`typedef` | [`std::function<bool(const Bytes&)> Predicate`](#callables-predicate)
`typedef` | [`std::function<std::string(const Bytes&)> Function`](#callables-function)
`typedef` | [`std::function<bool(const Bytes&, const Bytes&)> Compare`](#callables-compare)

<span class='declaration' id='callables-procedure'>`typedef std::function<void(const Bytes&)> Procedure`</span>

Types implementing this interface can process a value, but do not return a result. However, since objects of this type may have state, a procedure can be used to collect information about the processed data, and thus returning a result indirectly. See [std::function](http://en.cppreference.com/w/cpp/utility/functional/function) for more details on how to create such an object from a lambda expression, class method, or free function.

<span class='declaration' id='callables-predicate'>`typedef std::function<bool(const Bytes&)> Predicate`</span>

Types implementing this interface can process a value and return a boolean. Predicates check a value for certain property and thus, depending on the outcome, can be used to control the path of execution. See [std::function](http://en.cppreference.com/w/cpp/utility/functional/function) for more details on how to create such an object from a lambda expression, class method, or free function.

<span class='declaration' id='callables-function'>`typedef std::function<std::string(const Bytes&)> Function`</span>

Types implementing this interface can process a value and return a new one. Functions map an input value to an output value. An empty or no result can be signaled returning an empty string. `std::string` is used here as a convenient byte buffer that may contain arbitrary bytes. See [std::function](http://en.cppreference.com/w/cpp/utility/functional/function) for more details on how to create such an object from a lambda expression, class method, or free function.

<span class='declaration' id='callables-compare'>`typedef std::function<bool(const Bytes&, const Bytes&)> Compare`</span>

Types implementing this interface can process two values and return a boolean. Such functions determine the less than order of the given values according to the [Compare](http://en.cppreference.com/w/cpp/concept/Compare) concept. See [std::function](http://en.cppreference.com/w/cpp/utility/functional/function) for more details on how to create such an object from a lambda expression, class method, or free function.

## `class Iterator<bool>`

```cpp
#include <multimap/Iterator.hpp>
namespace multimap
```

This class template implements a forward iterator on a list of values. The template parameter decides whether an instantiation can delete values from the underlying list while iterating or not. If the actual parameter is `true` a const iterator will be created where the `DeleteValue()` method is disabled, and vice versa.

The iterator supports lazy initialization, which means that no IO operation is performed until one of the methods `SeekToFirst()`, `SeekTo(target)`, or `SeekTo(predicate)` is called. This might be useful in cases where multiple iterators have to be requested first to determine in which order they have to be processed.

The iterator also owns a lock for the underlying list to synchronize concurrent access. There are two types of locks: a reader lock (also called shared lock) and a writer lock (also called unique or exclusive lock). The former can be owned by a read-only iterator aka `Iterator<true>`, the latter can be owned by a read-write iterator aka `Iterator<false>`. The lock is automatically released when the lifetime of an iterator object ends and its destructor is called.

Users normally don't need to include or instantiate this class directly, but use the typedefs [`multimap::Map::ListIter`](#map-listiter) and [`multimap::Map::ConstListIter`](#map-constlistiter) instead.

Members       |
-------------:|-----------------------------------------------------------------
              | [`Iterator()`](#iterator-iterator)
`std::size_t` | [`NumValues() const`](#iterator-numvalues)
`void`        | [`SeekToFirst()`](#iterator-seektofirst)
`void`        | [`SeekTo(const Bytes& target)`](#iterator-seektotarget)
`void`        | [`SeekTo(Callables::Predicate predicate)`](#iterator-seektopredicate)
`bool`        | [`HasValue() const`](#iterator-hasvalue)
`Bytes`       | [`GetValue() const`](#iterator-getvalue)
`void`        | [`DeleteValue()`](#iterator-deletevalue)
`void`        | [`Next()`](#iterator-next)

<span class='declaration' id='iterator-iterator'>`Iterator()`</span>

Creates a default instance that has no values to iterate.

Postconditions:

* `NumValues() == 0`

<span class='declaration' id='iterator-numvalues'>`std::size_t NumValues() const`</span>

Returns the total number of values to iterate. This number does not change when the iterator moves forward. The method may be called at any time, even if `SeekToFirst()` or one of its friends have not been called.

<span class='declaration' id='iterator-seektofirst'>`void SeekToFirst()`</span>

Initializes the iterator to point to the first value, if any. This process will trigger disk IO if necessary. The method can also be used to seek back to the beginning of the list at the end of an iteration.

<span class='declaration' id='iterator-seektotarget'>`void SeekTo(const Bytes& target)`</span>

Initializes the iterator to point to the first value in the list that is equal to `target`, if any. This process will trigger disk IO if necessary.
   
<span class='declaration' id='iterator-seektopredicate'>`void SeekTo(Callables::Predicate predicate)`</span>

Initializes the iterator to point to the first value for which `predicate` yields `true`, if any. This process will trigger disk IO if necessary.
   
<span class='declaration' id='iterator-hasvalue'>`bool HasValue() const`</span>

Tells whether the iterator points to a value. If the result is `true`, the iterator may be dereferenced via `GetValue()`.
   
<span class='declaration' id='iterator-getvalue'>`Bytes GetValue() const`</span>

Returns the current value. The returned `Bytes` object wraps a pointer to data that is managed by the iterator. Hence, this pointer is only valid as long as the iterator does not move forward. Therefore, the value should only be used to immediately parse information or some user-defined object out of it. If an independent deep copy is needed you can call `Bytes::ToString()`.

Preconditions:

* `HasValue() == true`

<span class='declaration' id='iterator-deletevalue'>`void DeleteValue()`</span>

Deletes the value the iterator currently points to.

Preconditions:

* `HasValue() == true`

Postconditions:

* `HasValue() == false`

<span class='declaration' id='iterator-next'>`void Next()`</span>

Moves the iterator to the next value, if any.

## `class Map`

```cpp
#include <multimap/Map.hpp>
namespace multimap
```

This class implements a 1:n key-value store where each key is associated with a list of values.

Members        |
--------------:|----------------------------------------------------------------
`typedef`      | [`Iterator<false> ListIter`](#map-listiter)
`typedef`      | [`Iterator<true> ConstListIter`](#map-constlistiter)
               | [`Map()`](#map-map)
               | [`~Map()`](#map-~map)
               | [`Map(const boost::filesystem::path& directory, const Options& options)`](#map-map2)
`void`         | [`Open(const boost::filesystem::path& directory, const Options& options)`](#map-open)
`void`         | [`Put(const Bytes& key, const Bytes& value)`](#map-put)
`ConstListIter`| [`Get(const Bytes& key) const`](#map-get)
`ListIter`     | [`GetMutable(const Bytes& key)`](#map-getmutable)
`bool`         | [`Contains(const Bytes& key) const`](#map-contains)
`std::size_t`  | [`Delete(const Bytes& key)`](#map-delete)
`std::size_t`  | [`DeleteAll(const Bytes& key, Callables::Predicate predicate)`](#map-deleteall)
`std::size_t`  | [`DeleteAllEqual(const Bytes& key, const Bytes& value)`](#map-deleteallequal)
`bool`         | [`DeleteFirst(const Bytes& key, Callables::Predicate predicate)`](#map-deletefirst)
`bool`         | [`DeleteFirstEqual(const Bytes& key, const Bytes& value)`](#map-deletefirstequal)
`std::size_t`  | [`ReplaceAll(const Bytes& key, Callables::Function function)`](#map-replaceall)
`std::size_t`  | [`ReplaceAllEqual(const Bytes& key, const Bytes& old_value, const Bytes& new_value)`](#map-replaceallequal)
`bool`         | [`ReplaceFirst(const Bytes& key, Callables::Function function)`](#map-replacefirst)
`bool`         | [`ReplaceFirstEqual(const Bytes& key, const Bytes& old_value, const Bytes& new_value)`](#map-replacefirstequal)
`void`         | [`ForEachKey(Callables::Procedure procedure) const`](#map-foreachkey)
`void`         | [`ForEachValue(const Bytes& key, Callables::Procedure procedure) const`](#map-foreachvalue-procedure)
`void`         | [`ForEachValue(const Bytes& key, Callables::Predicate predicate) const`](#map-foreachvalue-predicate)
`std::map<std::string, std::string>` | [`GetProperties() const`](#map-getproperties)
`std::size_t`  | [`max_key_size() const`](#map-max-key-size)
`std::size_t`  | [`max_value_size() const`](#map-max-value-size)

Functions      |
--------------:|----------------------------------------------------------------
`void`         | [`Optimize(const boost::filesystem::path& from, const boost::filesystem::path& to)`](#map-optimize)
`void`         | [`Optimize(const boost::filesystem::path& from, const boost::filesystem::path& to, std::size_t new_block_size)`](#map-optimize2)
`void`         | [`Optimize(const boost::filesystem::path& from, const boost::filesystem::path& to, Callables::Compare compare)`](#map-optimize3)
`void`         | [`Optimize(const boost::filesystem::path& from, const boost::filesystem::path& to, Callables::Compare compare, std::size_t new_block_size)`](#map-optimize3)
`void`         | [`Import(const boost::filesystem::path& directory, const boost::filesystem::path& file)`](#map-import)
`void`         | [`Import(const boost::filesystem::path& directory, const boost::filesystem::path& file, bool create_if_missing)`](#map-import2)
`void`         | [`Import(const boost::filesystem::path& directory, const boost::filesystem::path& file, bool create_if_missing, std::size_t block_size)`](#map-import3)
`void`         | [`Export(const boost::filesystem::path& directory, const boost::filesystem::path& file)`](#map-export)

<span class='declaration' id='map-listiter'>`typedef Iterator<false> ListIter`</span>

An iterator type to iterate a mutable list. All operations declared in the class template `Iterator<bool>` that can mutate the underlying list are enabled.

<span class='declaration' id='map-constlistiter'>`typedef Iterator<true> ConstListIter`</span>

An iterator type to iterate a immutable list. All operations declared in the class template `Iterator<bool>` that can mutate the underlying list are disabled.

<span class='declaration' id='map-map'>`Map()`</span>

Creates a default instance which is not associated with a physical map.

<span class='declaration' id='map-~map'>`~Map()`</span>

If associated with a physical map the destructor flushes all data to disk and ensures that the map is stored in consistent state.

Preconditions:

* No list is in locked state, i.e. there is no iterator object pointing to an existing list.

<span class='declaration' id='map-map2'>
 `Map(const boost::filesystem::path& directory,`
 <script>space(9)</script> `const Options& options)`
</span>

Creates a new instance and opens the map located in `directory`. If the map does not exist and `options.create_if_missing` is set to `true` a new map will be created.

Throws `std::exception` if:

* `directory` does not exist.
* `directory` does not contain a map and `options.create_if_missing` is `false`.
* `directory` contains a map and `options.error_if_exists` is `true`.
* `options.block_size` is not a power of two.

<span class='declaration' id='map-open'>
 `void Open(`
 <script>space(20)</script>`const boost::filesystem::path& directory,`
 <script>space(20)</script>`const Options& options)`
</span>

Opens the map located in `directory`. If the map does not exist and `options.create_if_missing` is set to `true` a new map will be created.

Throws `std::exception` if:

* `directory` does not exist.
* `directory` does not contain a map and `options.create_if_missing` is `false`.
* `directory` contains a map and `options.error_if_exists` is `true`.
* `options.block_size` is not a power of two.

<span class='declaration' id='map-put'>`void Put(const Bytes& key, const Bytes& value)`</span>

Appends `value` to the end of the list associated with `key`.

Throws `std::exception` if:

* `key.size() > max_key_size()`
* `value.size() > max_value_size()`

<span class='declaration' id='map-get'>`ConstListIter Get(const Bytes& key) const`</span>

Returns a read-only iterator to the list associated with `key`. If no such mapping exists the list is considered to be empty. If the list is not empty a reader lock will be acquired to synchronize concurrent access to it. Thus, multiple threads can read the list at the same time. Once acquired, the lock is automatically released when the lifetime of the iterator ends and its destructor is called. If the list is currently locked exclusively by a writer lock, see `GetMutable()`, the method will block until the lock is released.

<span class='declaration' id='map-getmutable'>`ListIter GetMutable(const Bytes& key)`</span>

Returns a read-write iterator to the list associated with `key`. If no such mapping exists the list is considered to be empty. If the list is not empty a writer lock will be acquired to synchronize concurrent access to it. Only one thread can acquire a writer lock at a time, since it requires exclusive access. Once acquired, the lock is automatically released when the lifetime of the iterator ends and its destructor is called. If the list is currently locked, either by a reader or writer lock, the method will block until the lock is released.

<span class='declaration' id='map-contains'>`bool Contains(const Bytes& key)`</span>

Returns `true` if the list associated with `key` contains at least one value, returns `false` otherwise. If the key does not exist the list is considered to be empty. If a non-empty list is currently locked, the method will block until the lock is released.

<span class='declaration' id='map-delete'>`std::size_t Delete(const Bytes& key)`</span>

Deletes all values for `key` by clearing the associated list. This method will block until a writer lock can be acquired.

Returns: the number of deleted values.

<span class='declaration' id='map-deleteall'>
 `std::size_t DeleteAll(`
 <script>space(20)</script>`const Bytes& key,`
 <script>space(20)</script>`Callables::Predicate predicate)`
</span>

Deletes all values in the list associated with `key` for which `predicate` yields `true`. This method will block until a writer lock can be acquired.

Returns: the number of deleted values.

<span class='declaration' id='map-deleteallequal'>
 `std::size_t DeleteAllEqual(`
 <script>space(20)</script>`const Bytes& key,`
 <script>space(20)</script>`const Bytes& value)`
</span>

Deletes all values in the list associated with `key` which are equal to `value` according to `operator==(const Bytes&, const Bytes&)`. This method will block until a writer lock can be acquired.

Returns: the number of deleted values.

<span class='declaration' id='map-deletefirst'>
 `bool DeleteFirst(`
 <script>space(20)</script>`const Bytes& key,`
 <script>space(20)</script>`Callables::Predicate predicate)`
</span>

Deletes the first value in the list associated with `key` for which `predicate` yields `true`. This method will block until a writer lock can be acquired.

Returns: `true` if a value was deleted, `false` otherwise.

<span class='declaration' id='map-deletefirstequal'>
 `bool DeleteFirstEqual(`
 <script>space(20)</script>`const Bytes& key,`
 <script>space(20)</script>`const Bytes& value)`
</span>

Deletes the first value in the list associated with `key` which is equal to `value` according to `operator==(const Bytes&, const Bytes&)`. This method will block until a writer lock can be acquired.

Returns: `true` if a value was deleted, `false` otherwise.

<span class='declaration' id='map-replaceall'>
 `std::size_t ReplaceAll(`
 <script>space(20)</script>`const Bytes& key,`
 <script>space(20)</script>`Callables::Function function)`
</span>

Replaces all values in the list associated with `key` by the result of invoking `function`. If the result of `function` is an empty string no replacement is performed. A replacement does not happen in-place. Instead, the old value is marked as deleted and the new value is appended to the end of the list. Future releases will support in-place replacements. This method will block until a writer lock can be acquired.

Returns: the number of replaced values.

<span class='declaration' id='map-replaceallequal'>
 `std::size_t ReplaceAllEqual(`
 <script>space(20)</script>`const Bytes& key,`
 <script>space(20)</script>`const Bytes& old_value,`
 <script>space(20)</script>`const Bytes& new_value)`
</span>

Replaces all values in the list associated with `key` which are equal to `old_value` according to `operator==(const Bytes&, const Bytes&)` by `new_value`. A replacement does not happen in-place. Instead, the old value is marked as deleted and the new value is appended to the end of the list. Future releases will support in-place replacements. This method will block until a writer lock can be acquired.

Returns: the number of replaced values.

<span class='declaration' id='map-replacefirst'>
 `bool ReplaceFirst(`
 <script>space(20)</script>`const Bytes& key,`
 <script>space(20)</script>`Callables::Function function)`
</span>

Replaces the first value in the list associated with `key` by the result of invoking `function`. If the result of `function` is an empty string no replacement is performed. The replacement does not happen in-place. Instead, the old value is marked as deleted and the new value is appended to the end of the list. Future releases will support in-place replacements. This method will block until a writer lock can be acquired.

Returns: `true` if a value was replaced, `false` otherwise.

<span class='declaration' id='map-replacefirstequal'>
 `bool ReplaceFirstEqual(`
 <script>space(20)</script>`const Bytes& key,`
 <script>space(20)</script>`const Bytes& old_value,`
 <script>space(20)</script>`const` [`Bytes`](#class-bytes)`& new_value)`
</span>

Replaces the first value in the list associated with `key` which is equal to `old_value` according to `operator==(const Bytes&, const Bytes&)` by `new_value`. The replacement does not happen in-place. Instead, the old value is marked as deleted and the new value is appended to the end of the list. Future releases will support in-place replacements. This method will block until a writer lock can be acquired.

Returns: `true` if a value was replaced, `false` otherwise.

<span class='declaration' id='map-foreachkey'>
 `void ForEachKey(`
 <script>space(20)</script>`Callables::Procedure procedure) const`
</span>

Applies `procedure` to each key of the map whose associated list is not empty. For the time of execution the entire map is locked for read-only operations. It is possible to keep a reference to the map within `procedure` and to call other read-only operations such as `Get()`. However, trying to call mutable operations such as `GetMutable()` will cause a deadlock.

<span class='declaration' id='map-foreachvalue-procedure'>
 `void ForEachValue(`
 <script>space(20)</script>`const Bytes& key,`
 <script>space(20)</script>`Callables::Procedure procedure) const`
</span>

Applies `procedure` to each value in the list associated with `key`. This is a shorthand for requesting a read-only iterator via `Get(key)` followed by an application of `procedure` to each value obtained via `ConstListIter::GetValue()`. This method will block until a reader lock for the list in question can be acquired.

<span class='declaration' id='map-foreachvalue-predicate'>
 `void ForEachValue(`
 <script>space(20)</script>`const Bytes& key,`
 <script>space(20)</script>`Callables::Predicate predicate) const`
</span>

Applies `predicate` to each value in the list associated with `key` until `predicate` yields `false`. This is a shorthand for requesting a read-only iterator via `Get(key)` followed by an application of `predicate` to each value obtained via `ConstListIter::GetValue()` until `predicate` yields `false`. This method will block until a reader lock for the list in question can be acquired.

<span class='declaration' id='map-getproperties'>
 `std::map<std::string, std::string> GetProperties() const`
</span>

Returns a list of properties which describe the state of the map similar to those written to the `multimap.properties` file. This method will only look at lists which are currently not locked to be non-blocking. Therefore, the returned values will be an approximation. For the time of execution the map is locked for read-only operations.

<span class='declaration' id='map-max-key-size'>`std::size_t max_key_size() const`</span>

Returns the maximum size of a key in bytes which may be put. Currently this is `65536` bytes.

<span class='declaration' id='map-max-value-size'>`std::size_t max_value_size() const`</span>

Returns the maximum size of a value in bytes which may be put. Currently this is `options.block_size - 2` bytes.

<span class='declaration' id='map-optimize'>
 `void Optimize(`
 <script>space(20)</script>`const boost::filesystem::path& from,`
 <script>space(20)</script>`const boost::filesystem::path& to)`
</span>

Copies the map located in the directory denoted by `from` to the directory denoted by `to` performing the following optimizations:

* Defragmentation. All blocks which belong to the same list are written sequentially to disk which improves locality and leads to better read performance.
* Garbage collection. Values that are marked as deleted won't be copied which reduces the size of the new map and also improves locality.

Throws `std::exception` if:

* `from` or `to` are not directories.
* `from` does not contain a map.
* the map in `from` is locked.
* `to` already contains a map.
* `to` is not writable.

<span class='declaration' id='map-optimize2'>
 `void Optimize(`
 <script>space(20)</script>`const boost::filesystem::path& from,`
 <script>space(20)</script>`const boost::filesystem::path& to`
 <script>space(20)</script>`std::size_t new_block_size)`
</span>

Same as [`Optimize(from, to)`](#map-optimize) but sets the block size of the new map to `new_block_size`.

Throws `std::exception` if:

* see [`Optimize(from, to)`](#map-optimize)
* `new_block_size` is not a power of two.

<span class='declaration' id='map-optimize3'>
 `void Optimize(`
 <script>space(20)</script>`const boost::filesystem::path& from,`
 <script>space(20)</script>`const boost::filesystem::path& to,`
 <script>space(20)</script>`Callables::Compare compare)`
</span>

Same as [`Optimize(from, to)`](#map-optimize) but sorts each list before writing using `compare` as the sorting criterion.

Throws `std::exception` if:

* see [`Optimize(from, to)`](#map-optimize)

<span class='declaration' id='map-optimize4'>
 `void Optimize(`
 <script>space(20)</script>`const boost::filesystem::path& from,`
 <script>space(20)</script>`const boost::filesystem::path& to,`
 <script>space(20)</script>`Callables::Compare compare,`
 <script>space(20)</script>`std::size_t new_block_size)`
</span>

Same as [`Optimize(from, to, compare)`](#map-optimize3) but sets the block size of the new map to `new_block_size`.

Throws `std::exception` if:

* see [`Optimize(from, to, compare)`](#map-optimize3)
* `new_block_size` is not a power of two.

<span class='declaration' id='map-import'>
 `void Import(`
 <script>space(20)</script>`const boost::filesystem::path& directory,`
 <script>space(20)</script>`const boost::filesystem::path& file)`
</span>

Imports key-value pairs from a Base64-encoded text file denoted by `file` into the map located in the directory denoted by `directory`.

Preconditions:

* The content in `file` follows the format described in [Import / Export](overview.md#import-export).

Throws `std::exception` if:

* `directory` does not exist.
* `directory` does not contain a map.
* the map in `directory` is locked.
* `file` is not a regular file.

<span class='declaration' id='map-import2'>
 `void Import(`
 <script>space(20)</script>`const boost::filesystem::path& directory,`
 <script>space(20)</script>`const boost::filesystem::path& file`
 <script>space(20)</script>`bool create_if_missing)`
</span>

Same as [`Import(directory, file)`](#map-import) but creates a new map with default block size if the directory denoted by `directory` does not contain a map and `create_if_missing` is `true`.

Preconditions:

* see [`Import(directory, file)`](#map-import)

Throws `std::exception` if:

* see [`Import(directory, file)`](#map-import)

<span class='declaration' id='map-import3'>
 `void Import(`
 <script>space(20)</script>`const boost::filesystem::path& directory,`
 <script>space(20)</script>`const boost::filesystem::path& file,`
 <script>space(20)</script>`bool create_if_missing,`
 <script>space(20)</script>`std::size_t block_size)`
</span>

Same as [`Import(directory, file, create_if_missing)`](#map-import2) but sets the block size of a newly created map to `block_size`.

Preconditions:

* see [`Import(directory, file, create_if_missing)`](#map-import2)

Throws `std::exception` if:

* see [`Import(directory, file, create_if_missing)`](#map-import2)
* `block_size` is not a power of two.

<span class='declaration' id='map-export'>
 `void Export(`
 <script>space(20)</script>`const boost::filesystem::path& directory,`
 <script>space(20)</script>`const boost::filesystem::path& file)`
</span>

Exports all key-value pairs from the map located in the directory denoted by `directory` to a Base64-encoded text file denoted by `file`. If the file already exists, its content will be overridden.

Postconditions:

* The content in `file` follows the format described in [Import / Export](overview.md#import-export).

Throws `std::exception` if:

* `directory` does not exist.
* `directory` does not contain a map.
* the map in `directory` is locked.
* `file` cannot be created.

## `class Options`

```cpp
#include <multimap/Options.hpp>
namespace multimap
```

This class is a pure data holder used to configure an instantiation of class `Map`.

Members       |
-------------:|-----------------------------------------------------------------
`std::size_t` | [`block_size`](#options-block-size)
`bool`        | [`create_if_missing`](#options-create-if-missing)
`bool`        | [`error_if_exists`](#options-error-if-exists)
`bool`        | [`write_only_mode`](#options-write-only-mode)

<span class='declaration' id='options-block-size'>`std::size_t block_size`</span>

Determines the block size of a newly created map. The value is ignored if the map already exists when opened. The value must be a power of two. Have a look at [Choosing the block size](overview.md#choosing-the-block-size) for more information.

Default: `512`

<span class='declaration' id='options-create_if_missing'>`bool create_if_missing`</span>

Determines whether a map has to be created if it does not exist when opened.

Default: `false`

<span class='declaration' id='options-error_if_exists'>`bool error_if_exists`</span>

Determines whether an already existing map should be treated as an error when opened.

Default: `false`

<span class='declaration' id='options-write_only_mode'>`bool write_only_mode`</span>

Determines if the map should be opened in write-only mode. This will enable some optimizations for putting a large number of values, but will disable the ability to retrieve values. Users normally should leave this parameter alone.

Default: `false`

