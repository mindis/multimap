<div id="logo">Multimap</div>
<div>Overview Tutorial Reference Download</div>

<br />
**Multimap** is a 1:n key-value store optimized for large numbers of n. In this kind of store each key is associated with a list of values rather than a single one. Values can be appended to or removed from these lists, or just iterated quickly. Think of it as the [multimap data structure](https://en.wikipedia.org/wiki/Multimap) available in most programming languages, but with external persistent storage.

<div class="row">
<div class="col-md-6">
<h2>Features</h2>
<ul>
<li>Embeddable library with a clean C++ and Java interface.</li>
<li>Supported operations: Put, Get, Remove, Replace.</li>
<li>Keys and values are arbitrary byte arrays.</li>
<li>Keys are hold in memory, values are stored on disk.</li>
<li>Import/export from/to Base64-encoded text files.</li>
<li>Full thread-safe.</li>
</ul>
</div>
<div class="col-md-6">
<h2>Get Started</h2>
<ol>
<li>Read the [Overview](overview.md).</li>
<li>Get familiar with the [C++](cpptutorial.md) or [Java](javatutorial.md) tutorial.</li>
<li>[Download](download.md) and install the library.</li>
</ol>
</div>
</div>

Multimap is [free software](https://www.fsf.org/about/what-is-free-software) implemented in Standard C++11 and POSIX, licensed under the [GNU AGPL](http://www.gnu.org/licenses/agpl-3.0.en.html). The only supported platform at the moment is GNU/Linux. This is also true for the included Java binding.

<div class="row">
<div class="col-md-6">
<div class="panel panel-default">
  <div class="panel-heading">C++ Example</div>
  <div class="panel-body">test</div>
</div>
</div>

<div class="col-md-6">
<div class="panel panel-default">
  <div class="panel-heading">C++ Example</div>
  <div class="panel-body">test</div>
</div>
</div>
</div>

<!--
```cpp
#include <multimap/Map.hpp>

int main() {
  multimap::Map map;
  multimap::Options options;
  options.create_if_missing = true;
  map.open("/path/to/directory", options);

  map.put("key", "value 1");
  map.put("key", "value 2");

  auto iter = map.get("key");
  while (iter.hasNext()) {
    doSomething(iter.next());
  }
}
```
-->

<!--
```cpp
import io.multimap.Map;
import io.multimap.Options;

public static void main(String args[]) {
  Options options = new Options();
  options.setCreateIfMissing(true);
  Map map = new Map("/path/to/directory", options);
  
  map.put("key", "value 1");
  map.put("key", "value 2");

  Iterator<ByteBuffer> iter = map.get("key");
  while (iter.hasNext()) {
    doSomething(iter.next());
  }
}
```
-->
