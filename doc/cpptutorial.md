# C++ Tutorial

## Opening or Creating a Map

```cpp
#include <exception>
#include "multimap/Map.hpp"

multimap::Map map;
multimap::Options options;
options.block_size = 1024;  // Applies only if created.
options.create_if_missing = true;

try {
  map.Open("/path/to/directory", options);
} catch (std::exception& error) {
  HandleError(error);
}
```

If you want to throw an exception if the map already exists, you can set

```cpp
options.error_if_exists = true;
```

before calling `multimap::Map::Open`.

## Putting Values

## Getting Values

## Deleting Values

## Replacing Values

## Closing a Map

