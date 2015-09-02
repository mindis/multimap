<br />
<br />
<br />
**Multimap** is a 1:n key-value store optimized for large numbers of n. In this kind of store each key is associated with a list of values rather than a single one. Values can be appended to or removed from these lists, or just iterated quickly. Think of it as the [multimap data structure](https://en.wikipedia.org/wiki/Multimap) available in most programming languages, but with external persistent storage.

Multimap is [free software](https://www.fsf.org/about/what-is-free-software) implemented in Standard C++11 and POSIX, licensed under the [GNU AGPL](http://www.gnu.org/licenses/agpl-3.0.en.html). The only supported platform at the moment is GNU/Linux. This is also true for the included Java binding.

## Features

* Embeddable as a shared library with a clean C++ interface. Multimap has no server included.
* Keys and values are arbitrary byte arrays, so that you can use your favoured serialization method.
* Keys are hold in memory. Values are stored on disk.
* Supported operations: Put, Get, Delete, Replace.
* Java support included.
* Full thread-safe.

## How to Start

1. Read the [Guide](guide.md).
2. [Download](download.md) and build the library.
3. Start programming using the [C++](cpp-api.md) or [Java](java-api.md) reference.
