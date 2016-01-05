<br>

The following code snippets require

```java
import io.multimap.Iterator;
import io.multimap.Map;
import io.multimap.Options;
```

## Creating a Map

The minimal code to create a map is

```java
Options options = new Options();
options.setCreateIfMissing(true);
Map map = new Map("path/to/directory", options);
```

However, you should also catch errors that may occur during construction.

```java
Map map;
try {
  Options options = new Options();
  options.setCreateIfMissing(true);
  map = new Map("path/to/directory", options);
} catch (Exception e) {
  handleError(e);
}
```

Add `options.setErrorIfExists(true)` if you want to throw an exception if the map already exists.

## Opening a Map

To open an already existing map with default options you can simply omit the `options` parameter.

```java
Map map;
try {
  map = new Map("path/to/directory");
} catch (Exception e) {
  handleError(e);
}
```

If the map does not exist, an exception will be thrown.

## Closing a Map

A map must be explicitly closed by calling its `close` method in order to flush all data to disk and to ensure that the map is stored in consistent state. Not closing a map will cause data loss.

```java
map.close();  // Never forget.
```

## Putting Values

Putting a value means to append a value to the end of the list that is associated with a key. Both the key and the value must be of type `byte[]`.

```java
Charset cs = Charset.forName("UTF-8");

byte[] key = "key".getBytes(cs);

// Put a value constructed from a string.
map.put(key, "value".getBytes(cs));

// Put a value constructed from an integer.
map.put(key, Integer.toString(23).getBytes(cs));

// Put a value constructed from a java.nio.ByteBuffer.
map.put(key, ByteBuffer.allocate(4).putInt(23).array());

// Put a value packed by some serialization library.
map.put(key, somelib.serializeAsByteArray(someObject));
```

## Getting Values

Getting values means to request a read-only iterator to the list associated with a key. If no such list exists the returned iterator points to an empty list. An iterator that points to a non-empty list holds a reader lock on this list to synchronize concurrent access. A list can be read by multiple iterators or threads at the same time. If an iterator is not needed anymore its `close` method must be called in order to unlock the underlying list.

```java
Iterator iter = map.get(key);

while (iter.hasNext()) {
  ByteBuffer value = iter.next();
  doSomething(value);
}

iter.close();  // Releases the internal reader lock.
```

A call to `next` returns the next value as `java.nio.ByteBuffer`. This byte buffer is a direct one, which means that it is not backed by a `byte[]` managed by the Java Garbage Collector. Instead, it contains a pointer that points to memory allocated and managed by the native library. Hence, the value is only valid as long as the iterator is not moving forward. You should never copy around this kind of byte buffer.

Things about iterators that are also worth noting:

* Since iterators can be put into containers or the like, there has to take special care not to run into deadlocks, because there is another type of iterator which holds a writer lock on a list for exclusive access. This type of iterator is only used internally for implementing remove and replace operations.
* Iterators support lazy initialization which means that no I/O operation is performed until its `next` method has been called. This feature is useful in cases where multiple iterators must be requested first in order to determine in which order they have to be processed.
* Calling `nextAsByteArray` instead of `next` is a simple way to get a deep copy of the current value as a newly allocated `byte[]` that is managed by the Java Garbage Collector.
* Iterators can tell the number of values left to iterate via its `available` method.

## Removing Values

Values can be removed from a list by injecting a [predicate function](#) which yields `true` for values to be removed. There are methods to remove only the first or all matches from a list. Since this is a mutating operation the methods require exclusive access to the list and therefore try to acquire a writer lock for that list.

```java
import io.multimap.Callables;

map.put(key, "1".getBytes(cs));
map.put(key, "2".getBytes(cs));
map.put(key, "3".getBytes(cs));
map.put(key, "4".getBytes(cs));
map.put(key, "5".getBytes(cs));
map.put(key, "6".getBytes(cs));

map.removeValue(key, "3".getBytes(cs));
// Removes the first value equal to "3".
// key -> {"1", "2", "4", "5", "6"}

Predicate isEven = new Predicate() {  
  @Override
  protected boolean callOnReadOnly(ByteBuffer value) {
    return Integer.parseInt(Util.toString(value, cs)) % 2 == 0;
  }
};

map.removeValue(key, isEven);
// Removes the first value that is even.
// key -> {"1", "4", "5", "6"}

map.removeValues(key, isEven);
// Removes all values that are even.
// key -> {"1", "5"}

map.removeKey(key);
// Removes all values.
// key -> {}
```

When a value is removed it will only be marked as removed for the moment so that subsequent iterations will ignore it. Only an optimize operation triggered either via [API call](#) or the [command line tool](overview#multimap-optimize) will remove the data physically.

Note that if the list is already locked either by a reader or writer lock the methods will block until the lock is released. A thread should never try to remove values while holding iterators to avoid deadlocks.

## Replacing Values

Values can be replaced by injecting a [map function](#) which yields a `byte[]` for values to be replaced or `null` if not. There are methods to replace only the first or all matches in a list. Since this is a mutating operation the methods require exclusive access to the list and therefore try to acquire a writer lock for that list.

When a value is replaced the old value is marked as removed and the new value is appended to the end of the list. Hence, replace operations will not preserve the order of values. If a certain order needs to be restored an [optimize operation](#) has to be run parameterized with an appropriate [compare function](#).

```java
import io.multimap.Callables;

map.put(key, "1".getBytes(cs));
map.put(key, "2".getBytes(cs));
map.put(key, "3".getBytes(cs));
map.put(key, "4".getBytes(cs));
map.put(key, "5".getBytes(cs));
map.put(key, "6".getBytes(cs));

map.replaceValue(key, "3".getBytes(cs), "4".getBytes(cs));
// Replaces the first value equal to "3" by "4".
// key -> {"1", "2", "4", "5", "6", "4"}

Function incrementIfEven = new Function() {  
  @Override
  protected byte[] callOnReadOnly(ByteBuffer value) {
    int number = Integer.parseInt(Util.toString(value, cs));
    return (number % 2 == 0) ? Integer.toString(number + 1).getBytes(cs) : null;
  }
};

map.replaceValue(key, incrementIfEven);
// Replaces the first value that is even by its successor.
// key -> {"1", "4", "5", "6", "4", "3"}

map.replaceValues(key, "4".getBytes(cs), "8".getBytes(cs));
// Replaces all values equal to "4" by "8".
// key -> {"1", "5", "6", "3", "8", "8"}

map.replaceValues(key, incrementIfEven);
// Replaces all values that are even by its successor.
// key -> {"1", "5", "3", "7", "9", "9"}
```

Note that if the list is already locked either by a reader or writer lock the methods will block until the lock is released. A thread should never try to replace values while holding iterators to avoid deadlocks.
