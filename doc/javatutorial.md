## Opening a Map

An already existing map can be opened calling the constructor `io.multimap.Map#Map(directory, options)`. If you want to create a new map in case it does not exist, you can do so via `options.setCreateIfMissing(true)`.

```java
import io.multimap.Map;
import io.multimap.Options;
import java.nio.file.Paths;

Map map = null;
Options options = new Options();
options.setBlockSize(1024);  // Applies only if created.
options.setCreateIfMissing(true);
    
try {
  map = new Map(Paths.get("/path/to/directory"), options);
} catch (Exception error) {
  handleError(error);
}
```

If you want an exception to be thrown if the map already exists, you can do so via `options.setErrorIfExists(true)`.

## Closing a Map

A map must be closed via `io.multimap.Map#close()` to flush all data to disk and to ensure that the map is stored in consistent state. Not closing a map will cause data loss.

```java
Map map = new Map(Paths.get("/path/to/directory"), options);
doSomething(map);
map.close();  // Never forget.
```

## Putting Values

Putting a value means to append a value to the end of the list that is associated with a key. Both objects, the key and the value, must be of type `byte[]`.

```java
import java.nio.ByteBuffer;

byte[] key = "key".getBytes();

// Put a value constructed from a java.lang.String.
byte[] value = "value".getBytes();
map.put(key, value);

// Put a value constructed from a java.nio.ByteBuffer.
value = ByteBuffer.allocate(4).putInt(23).array();
map.put(key, value);

// Put a value constructed from some serialization library.
value = somelib.serialize(someObject);
map.put(key, value);
```

## Getting Values

Getting values means to request an iterator to the list that is associated with a key. The returned iterator can be mutable or immutable. The latter does not allow to delete values while iterating. If for some key no such list exists the iterator points to an empty list.

An iterator that points to a non-empty list also holds a lock on this list to synchronize concurrent access. There are two types of locks, a reader and a writer lock. A reader lock is acquired by an immutable iterator while a writer lock is acquired by a mutable iterator. A list can be locked by multiple reader locks, i.e. threads, at the same time while a writer lock requires exclusive access. The lock is released when the iterator is closed via `io.multimap.Iterator#close()`. Not closing iterators will cause deadlocks.

```java
import io.multimap.Iterator;

Iterator iter = map.get(key);

for (iter.seekToFirst(); iter.hasValue(); iter.next()) {
  ByteBuffer value = iter.getValue();
  doSomething(value);
}
iter.close();  // Releases internal list lock.
```

Iterators allow for lazy initialization which means that no IO operation is performed until you call `seekToFirst()` or one of its friends `seekTo(target)` and `seekTo(predicate)`. This might be useful in cases where you need to request multiple iterators first to determine in which order they have to be processed. The length of the underlying list can be obtained via `numValues()` even if the iterator has not been initialized.

A call to `getValue()` returns the value the iterator currently points to as `ByteBuffer`. This byte buffer is a direct one, which means that it is not backed by a `byte[]` managed by the Java Garbage Collector. Instead, it contains a pointer that points to memory allocated and managed by the shared library. Hence, the value is only valid as long as the iterator is not moved forward. You should never copy around this kind of byte buffer.

To get an independent copy of the value that is managed by the Java GC you can call `getValueAsByteArray()`. However, for performance reasons, you should try to parse the content directly out of the byte buffer. Most [serialization libraries](https://en.wikipedia.org/wiki/Comparison_of_data_serialization_formats) should support this operation.

## Deleting Values

When a value is deleted it will initially be marked as deleted so that it will be ignored by any subsequent operation. Only the static methods `io.multimap.Map#Optimize` or `io.multimap.Map#Export` will remove the data physically.

Values can be deleted using a mutable iterator:

```java
Iterator iter = map.getMutable(key);

for (iter.seekToFirst(); iter.hasValue(); iter.next()) {
  if (canBeDeleted(iter.getValue())) {
    iter.deleteValue();
    // iter.hasValue() is false now.
    // iter.next() will seek to the next value, if any.
  }
}
```

or using the following methods provided by class `io.multimap.Map`:

* `long delete(key)`
* `long deleteAll(key, predicate)`
* `long deleteAllEqual(key, value)`
* `boolean deleteFirst(key, predicate)`
* `boolean deleteFirstEqual(key, value)`

Example 1: Delete every value (of type `int`) that is even.

```java
import io.multimap.Callables.Predicate;

Predicate isEven = new Predicate() {
  @Override
  protected boolean callOnReadOnly(ByteBuffer value) {
    int number = value.getInt();
    return number % 2 == 0;
  }
};

// Before: key -> (0, 1, 2, 3, 4, 5, 6, 7, 8, 9)
long numDeleted = map.deleteAll(key, isEven);
// After: key -> (1, 3, 5, 7, 9)
// numDeleted == 5
```

Example 2: Delete every value (of type `int`) that is equal to `23`.

```java
int number = 23;
byte[] value = ByteBuffer.allocate(4).putInt(number).array();

// Before: key -> (0, 1, 23, 45, 67, 89, 23)
long numDeleted = map.deleteAllEqual(key, value);
// After: key -> (0, 1, 45, 67, 89)
// numDeleted == 2
```

## Replacing Values

When a value is replaced the old value is marked as deleted and the new value is appended to the end of the list. Hence, replacing operations will not preserve the order of values. There is an open [issue](https://bitbucket.org/mtrenkmann/multimap/issues/2) to support in-place replacements in future releases of library.

Values can be replaced using the following methods provided by class `io.multimap.Map`:

* `long replaceAll(key, function)`
* `long replaceAllEqual(key, oldValue, newValue)`
* `boolean replaceFirst(key, function)`
* `boolean replaceFirstEqual(key, oldValue, newValue)`

Example 1: Replace every value (of type `int`) that is even by its natural successor.

```java
import io.multimap.Callables.Function;

Function incrementIfEven = new Function() {
  @Override
  protected byte[] callOnReadOnly(ByteBuffer value) {
    int number = value.getInt();
    byte[] result = null;
    if (number % 2 == 0) {
      ++number;
      result = ByteBuffer.allocate(4).putInt(number).array();
    }
    return result;
  }
};

// Before: key -> (0, 1, 2, 3, 4, 5, 6, 7, 8, 9)
long numReplaced = map.replaceAll(key, incrementIfEven);
// After: key -> (1, 3, 5, 7, 9, 1, 3, 5, 7, 9)
// numReplaced == 5
```

Example 2: Replace every value (of type `int`) that is equal to `23` by `42`.

```cpp
int oldNumber = 23;
int newNumber = 42;
byte[] oldValue = ByteBuffer.allocate(4).putInt(oldNumber).array();
byte[] newValue = ByteBuffer.allocate(4).putInt(newNumber).array();

// Before: key -> (0, 1, 23, 45, 67, 89, 23)
long numReplaced = map.replaceAllEqual(key, oldValue, newValue);
// After: key -> (0, 1, 45, 67, 89, 42, 42)
// numReplaced == 2
```

