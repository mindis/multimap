# Java Reference

<br>
```java
import io.multimap.Map;
import io.multimap.Options;
import io.multimap.Iterator;
```

In progress - please try again tomorrow ;-)

<!--
## Map.Map

`Map(Path directory, Options options) throws Exception`

Constructor that opens or creates a map in `directory`.

Throws: `java.lang.Exception` if something went wrong.

```java
Options options = new Options();
options.setBlockSize(512);
options.setBlockPoolMemory(GiB(2));
options.setCreateIfMissing(true);

multimap::Map map;
map.Open("/path/to/multimap", options);
```
-->

