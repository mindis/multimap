<br>

Multimap is a fast 1:n key-value store that provides a mapping from keys to lists of values. Think of it as the <a href="https://en.wikipedia.org/wiki/Multimap" target="_blank">multimap data structure</a> available in most programming languages, but with external persistent storage. Multimap is optimized for large numbers of n and works perfectly as a building block for retrieval systems that make use of <a href="https://en.wikipedia.org/wiki/Inverted_index" target="_blank">inverted indexing</a>.

<div class="row">
  <div class="col-md-6">
    <h2>Features</h2>
    <ul>
    <li>Embeddable store with a clean <a href="cppreference">C++</a> and <a href="javareference">Java interface</a>.</li>
    <li>Supported operations: put, get, remove, replace.</li>
    <li>Keys and values are arbitrary byte arrays.</li>
    <li>Keys are hold in memory, values are stored on disk.</li>
    <li>Import/export from/to Base64-encoded text files.</li>
    <li>Full thread-safe.</li>
    </ul>
  </div>
  <div class="col-md-6">
    <h2>Get Started</h2>
    <ul>
    <li>Read the <a href="overview/">overview</a> and learn the basics.</li>
    <li>Try the <a href="cpptutorial">C++</a> or <a href="javatutorial">Java tutorial</a> to get familiar with the API.</li>
    <br>
    <a class="btn btn-default btn-lg" href="downloadv03/" role="button"><span class="glyphicon glyphicon-download-alt" aria-hidden="true"></span>&nbsp;&nbsp;Download Release 0.3</a>
    </ul>
  </div>
</div>
<br>
<div class="row">
<div class="col-md-6">
<div class="panel panel-default">
<div class="panel-heading">C++ Example</div>
<div class="panel-body">
```cpp
#include <multimap/Map.hpp>

using namespace multimap;

int main() {
  Options options;
  options.create_if_missing = true;
  Map map("some/directory", options);

  map.put("key", "1st value");
  map.put("key", "2nd value");

  auto iter = map.get("key");
  while (iter.hasNext()) {
    doSomething(iter.next());
  }
  
  // Just to line up
  // with the Java box.
}
```
</div>
</div>
</div>
<div class="col-md-6">
<div class="panel panel-default">
<div class="panel-heading">Java Example</div>
<div class="panel-body">
```java
import io.multimap.Map;
import io.multimap.Options;
import io.multimap.Iterator;

public static void main(String[] args) {
  Options options = new Options();
  options.setCreateIfMissing(true);
  Map map = new Map("some/directory", options);

  map.put("key", "1st value");
  map.put("key", "2nd value");

  Iterator iter = map.get("key");
  while (iter.hasNext()) {
    doSomething(iter.next());
  }
  
  iter.close();
  map.close();
}
```
</div>
</div>
</div>
</div>
<br>

Multimap is <a href="https://www.fsf.org/about/what-is-free-software" target="_bank">free software</a> implemented in standard C++11 and POSIX, distributed under the terms of the <a href="http://www.gnu.org/licenses/agpl-3.0.en.html" target="_blank">GNU Affero General Public License</a> (AGPL) version 3. The only supported platform at the moment is GNU/Linux on x86/32 and x86/64. This is also true for the included Java binding.
