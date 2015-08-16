<br />
<br />
<br />
**Multimap** is a 1:n key-value store optimized for large numbers of n. Originally developed as a light-weight implementation of the [inverted index data structure](https://en.wikipedia.org/wiki/Inverted_index) commonly used in information retrieval systems, it matured into a general-purpose persistent map where a key is associated with n values.

Multimap is [free software](https://www.fsf.org/about/what-is-free-software) implemented in Standard C++11 licensed under the [GNU AGPL](http://www.gnu.org/licenses/agpl-3.0.en.html). Although it comes with a Java language binding onboard, its only supported target platform at the moment is GNU/Linux.

## Features

* Keys and values are arbitrary byte arrays.
* Keys are hold in memory. Values are stored on disk.
* Supported operations: Put, Get, Delete, Replace.
* Java support included.
* Full thread-safe.

## Limitations

Since all keys are hold in memory their number is limited by the amount of available memory. The maximum size of a single key is 65536 bytes. As an example, a key set of one million English dictionary words, each word five characters long on average, will consume approximately 13 MiB (5 MiB data plus 8 MiB management overhead) of memory.

The maximum size of a single value is limited by the configured block size. Since values are physically organized in blocks, a value must fit completely into one block. Typical block sizes are 128, 256, 512, 1024, and so on. As a rule of thumb, there should be room for at least 10 values in a block.

<!---
## Donate
-->
