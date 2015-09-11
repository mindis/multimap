## How it works

Multimap is a 1:n key-value store implemented as an in-memory hash table which maps each key to a list of values. Keys and values are arbitrary byte arrays. The following schema illustrates the general design:

```
"a" -> (1, 2, 3, 4, 5, 6)
"b" -> (7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17)
"c" -> (18, 19, 20, 21, 22, 23, 24)
```

The lists on the right side are organized logically in blocks that have a fixed size.

```
"a" -> [1, 2, 3], [4, 5, 6]
"b" -> [7, 8, 9], [10, 11, 12], [13, 14, 15], [16, 17, free]
"c" -> [18, 19, 20], [21, 22, 23], [24, free]
```

The keys on the left side, as well as the last block of each list on the right side are hold in memory. All other blocks are stored on disk. The in-memory blocks are used as write buffers which are written to disk when there is no more room for the next value to put. On the other hand, in a read-only scenario no blocks will be allocated at all.

## Limitations

The number or size of keys you can put into a map is limited by the amount of available memory, because the entire key set is hold in-memory. Furthermore, each list will allocate room for its last block as soon as a value is put. For initial data imports this means that Multimap needs to allocate the same number of blocks as there are keys in the key set.

For example, consider a key set of one million English words, each word five characters long on average, and a block size of 512 bytes. Such a map will consume about 570 MiB (1M * (5 + 512 + 80)) of memory (80 bytes per key management overhead). If the map is used read-only it needs less.

## Choosing the block size

Although, it is possible that a single value covers an entire block, this setup is far from optimal and should be avoided. Instead, the block size should be as large as possible to provide room for several values. In general, this leads to better performance since more data can be carried at once. However, there is a tradeoff. If the block size is large and your key set is also large, you will run out of memory. Try to make estimates similar to the example above. Typical block sizes are 128, 256, 512, 1024, and so on (yes, it must be a power of two).

Certainly, this design favours small values sizes, but this is intended. Multimap originated from an [inverted index](https://en.wikipedia.org/wiki/Inverted_index) implementation where the main focus is on storing integers that represent things like document ids and word counts. Multimap is not suitable and was not designed to store large binary objects.

To figure out whether the chosen block size was appropriate you can check the average load factor of all written blocks. This property can either be obtained via [`multimap::Map::GetProperties()`](cppreference.md#map-getproperties) or read from the `multimap.properties` file that is generated in the directory of the map. If it turns out that the block size was not optimal you can

* create a new map from scratch trying another block size,
* or run [`multimap::Optimize`](cppreference.md#map-optimize) to generate a new map.

## Serialization

Keys and values are arbitrary byte arrays. This has the advantage that Multimap does not need to deal with serialization and leaves the door open for compression (currently not applied). For you, the user, this has the advantage that you can stick with your favoured serialization method. For very basic data types like primitives or [PODs](http://en.cppreference.com/w/cpp/concept/PODType) a simple `std::memcpy` will be sufficient. For more complex objects there are plenty of libraries available:

Wikipedia: [Comparison of data serialization formats](https://en.wikipedia.org/wiki/Comparison_of_data_serialization_formats)

## Import / Export

Key-value pairs can be imported from or exported to Base64-encoded text files using `multimap::Import` or `multimap::Export`, respectively. This feature is useful for data exchange to/from other libraries and frameworks, or for backup purposes. The file format is defined as follows:

* The file is in [CSV format](https://en.wikipedia.org/wiki/Comma-separated_values).
* The delimiter is whitespace or tab.
* Each line starts with a key followed by one or more values.
* Multiple lines may start with the same key.
* Keys and values are Base64-encoded.

Example:

```text
key1 value1
key2 value2 value3
key1 value4
key3 value5 value6 value7 value8
key2 value9
```

is equivalent to:

```text
key1 value1 value4
key2 value2 value3 value9
key3 value5 value6 value7 value8
```

is equivalent to:

```text
key1 value1
key1 value4
key2 value2
key2 value3
key2 value9
key3 value5
key3 value6
key3 value7
key3 value8
```

## Optimization

An existing Multimap can be optimized using one of the `multimap::Optimize` functions. The optimize operation performs a rewrite of the entire map and therefore, depending on the size of the map, might be a long running task. Optimization has the following effects:

* **Defragmentation**. All blocks which belong to the same list are written sequentially to disk which improves locality and leads to better read performance.
* **Garbage collection**. Values that are marked as deleted won't be copied which reduces the size of the new map and also improves locality.

Optional:

* **Sorting**. All lists can be sorted applying a user-defined `multimap::Callables::Compare` function.
* **Block size**. The new optimized map can be given a different and maybe more suitable block size.

## Using as 1:1 key-value store

Multimap can be used as a 1:1 key-value store as well, although other libraries may be better suited for this purpose. As always, you should pick the library that best fits your needs, both with respect to features and performance. Finally, when using Multimap as a 1:1 key-value store, you should set the block size as small as possible to waste as little space as possible, because each block will only contain just one value. Also keep in mind that the block size defines the maximum size of a value.

<!--
## Benchmarks
## Set Operations
-->

