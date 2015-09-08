## How it works

Multimap is a 1:n key-value store. It is implemented as an in-memory hash table that maps each key to a list of values. Keys and values are arbitrary byte arrays. Consider the following schema:

```
"a" -> (1, 2, 3, 4, 5, 6)
"b" -> (7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17)
"c" -> (18, 19, 20, 21, 22, 23, 24)
```

The lists on the right side are organized logically in blocks that have a fixed size.

```txt
"a" -> [1, 2, 3], [4, 5, 6]
"b" -> [7, 8, 9], [10, 11, 12], [13, 14, 15], [16, 17, free]
"c" -> [18, 19, 20], [21, 22, 23], [24, free]
```

The key set on the left side, as well as the last block of each list on the right side are hold in memory. All other blocks are stored on disk. Strictly speaking, this is only true for a write scenario. If you open an already existing map, no blocks will be allocated until the first value is put. Think of the last block as a write buffer.

## Limitations

Since all keys are hold in memory, their number is limited by the amount of available memory. Furthermore, as just mentioned each list allocates room for its last block when at least one value is put. For initial data imports this means that Multimap needs to allocate the same number of blocks as there are keys in the key set.

For example, consider a key set of one million English words, each word five characters long on average, and a block size of 512 bytes. Such a map will consume about 570 MiB (1M * (5 + 512 + 80)) of memory (80 bytes per key management overhead). If the map is used read-only it needs less.

## Choosing the block size

Although, it is possible that a single value covers an entire block, this setup is far from optimal and should be avoided. Instead, the block size should be as large as possible to provide room for several values. In general, this leads to better performance since more data can be carried at once. However, there is a tradeoff. If the block size is large and your key set is also large, you will run out of memory. Try to make estimates similar to the example above. Typical block sizes are 128, 256, 512, 1024, and so on (yes, it must be a power of two).

Certainly, this design favours small values sizes, but this is intended. Multimap originated from an [inverted index](https://en.wikipedia.org/wiki/Inverted_index) implementation where the main focus is on storing integers that represent things like document ids and word counts. Multimap is not suitable and was not designed to store large binary objects.

To figure out whether the chosen block size was appropriate you can check the average load factor of all written blocks. This property can either be obtained via `Map::GetProperties()` or read from the `multimap.properties` file that is generated in the directory of the map. If it turns out that the block size was not optimal you can

* create a new map trying another block size.
* call `multimap::Optimize("/path/to/old/map", "/path/to/new/map", new_block_size)`.
* create a new map importing your data from base64-encoded text files.

## Serialization

Keys and values are arbitrary byte arrays. This has the advantage that Multimap does not need to deal with serialization and leaves the door open for compression (currently not applied). For you, the user, this has the advantage that you can stick with your favoured serialization method. For very basic data types like primitives or [PODs](http://en.cppreference.com/w/cpp/concept/PODType) a simple [memcpy](http://en.cppreference.com/w/cpp/string/byte/memcpy) will be sufficient. For more complex objects there are plenty of libraries available:

Wikipedia: [Comparison of data serialization formats](https://en.wikipedia.org/wiki/Comparison_of_data_serialization_formats)

## Import / Export



## Optimization

## Using as 1:1 key-value store

<!--
## Benchmarks
## Set Operations
-->

