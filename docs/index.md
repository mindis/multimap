**InvertedIndex** is a light-weight implementation of the [index data structure](https://en.wikipedia.org/wiki/Inverted_index) commonly used in information retrieval systems. Although its main purpose is to provide a building block for full text search engines it can be used as a general-purpose on-disk key-value store where one key is mapped to n values. InvertedIndex is free software, implemented in C++, and comes with a Java binding onboard.

## Rationale

The heart of every information retrieval system is its index. The purpose of such an index is to map a query artefact, e.g. a word, to a number of values, e.g. ids of documents where the word occurs in. At query processing time, reading data out of the index is a crucial performance factor, especially when multiple keys need to be looked up and their values need to be merged or intersected.

With the emergence of the NoSQL paradigm, [new types of databases](https://en.wikipedia.org/wiki/Nosql#Types_of_NoSQL_databases) have been established. The most promising candidates to serve as an inverted index is the class of embedded key-value stores -- libraries ready to be integrated into larger systems.

However, there is no embedded key-value store we know of, that implements a 1:n mapping with read/write access at reasonable speed. Some libraries explicitly allow to put arrays as a single value, but treat them as blob data internally, making it impossible to append or delete individual values to/from these arrays.

The **InvertedIndex** library fills this gap. It provides the functionality of a multimap, associating one key with a list of n values, plus persistent data storage. Optimized for small values and large numbers of n, appending or deleting values to/from a list is possible at any time. Looking up and iterating a list is super-fast.

## Features

* Keys and values are arbitrary byte arrays.
* Keys are hold in memory. Values are stored on disk.
* Supported operations: Put, Get, Delete, Update.
* Java support included.
* Full thread-safe.

## Limitations

Since all **keys** are hold in memory the number of keys is bounded by the amount of available memory. The size of a single key is practically unbounded. As an example, having a key set of 1M English dictionary words, each word five characters long on average, will consume approximately XX megabytes of memory.

The number of **values** that could be put into a single index instance is physically limited by the maximum file size of the underlying file system. Modern file systems support file sizes that exceed the capacity of most storage devices. A single value, however, can have a maximum size of **127 bytes**.

Block size

## Serialization

## Benchmarks

## Download

<!---
## Donate
-->
