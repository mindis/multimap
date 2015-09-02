## How it Works

Multimap is a 1:n key-value store. It is implemented as an in-memory hash table that maps each key to a list of values. Keys and values are arbitrary byte arrays. Consider the following schema:

```txt
"key1" -> (1, 2, 3, 4, 5, 6)
"key2" -> (7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17)
"key3" -> (18, 19, 20, 21, 22, 23, 24)
```

The lists on the right side are organized logically in blocks of fixed size. We will refer to this parameter as the block size of the map.

```txt
"key1" -> [1, 2, 3], [4, 5, 6]
"key2" -> [7, 8, 9], [10, 11, 12], [13, 14, 15], [16, 17, free]
"key3" -> [18, 19, 20], [21, 22, 23], [24, free]
```

The key set on the left side, as well as the first block of each list on the right side are hold in memory. All other blocks are stored on disk. This is at least true for the write scenario. If you open an already existing map, no blocks will be allocated until you put the first value. Think of the first block as a write buffer.

## Limitations

Since all keys are hold in memory, their number is limited by the amount of available memory. Furthermore, each list where at least one value is put allocates room for its first block. For initial data imports this means that Multimap allocates the same number of blocks as there are keys in the key set.

As an example, consider a key set of one million English dictionary words, where each word is five characters long on average, and a block size of 512 bytes. Such a map, if not used entirely read-only, will consume at most ~500 MiB (1M * (5 + 512)) of memory at runtime.

The block size also limits the maximum size of a single value, because it must fit entirely into a block. If the block size is 512 bytes, the maximum size of a value is 510 bytes. It is possible, though far from optimal, that a single value covers an entire block, but it can never span multiple blocks. Certainly, this design favours small values sizes, but this is intended. Multimap originated from a [inverted index](https://en.wikipedia.org/wiki/Inverted_index) implementation, where values are integers representing doc ids and word counts. Multimap is not suitable for storing large binary objects.

### Choosing the block size

The block size will be specified when creating a map. It can later be changed via an optimize routine. The block size should be compatible with the average size of the values to be put. If the block size is too small, just a few, only one, or even no value at all will fit into a block. If the block size is too large, you will run out of memory, if you also have a large key set. For best performance the block size should be as large as possible, to carry more data at once. You can find the average load factor of the blocks written in the map's properties file. Typical block sizes are 128, 256, 512, 1024, and so on.

## Serialization

As mentioned before, keys and values are arbitrary byte arrays. This has the advantage that Multimap does not need to deal with serialization, and leaves the door open for compression (currently not applied). For the user this has the advantage that he/she can stick with his/her favoured serialization method.

For very basic data types, such as primitives or [PODs](http://en.cppreference.com/w/cpp/concept/PODType), a simple [std::memcpy](http://en.cppreference.com/w/cpp/string/byte/memcpy) will be sufficient. For more complex objects there are plenty of libraries available:

Wikipedia: [Comparison of data serialization formats](https://en.wikipedia.org/wiki/Comparison_of_data_serialization_formats)

## Import / Export

## Optimization

## Getting Started

<!--
## Benchmarks
-->

<!--
## Set Operations
-->
