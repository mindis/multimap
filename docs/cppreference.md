## Bytes.hpp

```cpp
#include <multimap/Bytes.hpp>
namespace multimap
```

### class Bytes

This class is a thin wrapper for raw binary data. It just holds a pointer to data together with its size (number of bytes). It is used to represent keys and values that are put into or gotten from a map. An object of this class never deep copies the data it is constructed from, nor does it take any ownership of the data. It is really just a helper for providing a generic interface.

<table class="reference-table">
<tbody>
 <tr>
  <th colspan="2">Member functions</th>
 </tr>
 <tr>
  <td></td>
  <td>
   <code>Bytes()</code>
   <div>Creates an empty byte array. <a href="#bytes-bytes">more...</a><div>
  </td>
 </tr>
 <tr>
  <td></td>
  <td>
   <code>Bytes(const char * cstr)</code>
   <div>Creates a byte array from a null-terminated C-string. <a href="#bytes-bytes-cstr">more...</a><div>
  </td>
 </tr>
 <tr>
  <td></td>
  <td>
   <code>Bytes(const std::string & str)</code>
   <div>Creates a byte array from a standard string. <a href="#bytes-bytes-str">more...</a><div>
  </td>
 </tr>
 <tr>
  <td></td>
  <td>
   <code>Bytes(const void * data, size_t size)</code>
   <div>Creates a byte array from arbitrary data. <a href="#bytes-bytes-data-size">more...</a><div>
  </td>
 </tr>
 <tr>
  <td><code>const char *</code></td>
  <td>
   <code>data() const</code>
   <div>Returns a read-only pointer to the wrapped data.<div>
  </td>
 </tr>
 <tr>
  <td><code>size_t</code></td>
  <td>
   <code>size() const</code>
   <div>Returns the number of bytes wrapped.<div>
  </td>
 </tr>
 <tr>
  <td><code>const char *</code></td>
  <td>
   <code>begin() const</code>
   <div>Returns a read-only pointer to the beginning of the wrapped data.<div>
  </td>
 </tr>
 <tr>
  <td><code>const char *</code></td>
  <td>
   <code>end() const</code>
   <div>Returns a past-the-end pointer to the wrapped data.<div>
  </td>
 </tr>
 <tr>
  <td><code>bool</code></td>
  <td>
   <code>empty() const</code>
   <div>Tells whether the byte array is empty. <a href="#bytes-empty">more...</a><div>
  </td>
 </tr>
 <tr>
  <td><code>void</code></td>
  <td>
   <code>clear()</code>
   <div>Let this byte array point to an empty array. <a href="#bytes-clear">more...</a><div>
  </td>
 </tr>
 <tr>
  <td><code>std::string</code></td>
  <td>
   <code>toString() const</code>
   <div>Returns a string which contains a copy of the wrapped data. <a href="#bytes-tostring">more...</a><div>
  </td>
 </tr>
 <tr>
  <th colspan="2">Non-member functions</th>
 </tr>
 <tr>
  <td><code>bool</code></td>
  <td>
   <code>operator==(const Bytes & lhs, const Bytes & rhs)</code>
   <div>Compares two byte arrays for equality. <a href="#operator-eq">more...</a><div>
  </td>
 </tr>
 <tr>
  <td><code>bool</code></td>
  <td>
   <code>operator!=(const Bytes & lhs, const Bytes & rhs)</code>
   <div>Returns the inverse of the previous function.<div>
  </td>
 </tr>
 <tr>
  <th colspan="2">Helper classes</th>
 </tr>
 <tr>
  <td></td>
  <td>
   <code>std::hash&lt;Bytes&gt;</code>
   <div>Hash support for class Bytes.<div>
  </td>
 </tr>
</tbody>
</table>

<div class="reference-more">
 <h4 id="bytes-bytes"><code>Bytes::Bytes()</code></h4>
 <p>Creates an empty byte array.</p>
 <p><span class="ensures" /></p>
 <ul>
  <li><code>data() != nullptr</code></li>
  <li><code>size() == 0</code></li>
 </ul>
</div>

<div class="reference-more">
 <h4 id="bytes-bytes-cstr"><code>Bytes::Bytes(const char * cstr)</code></h4>
 <p>Creates a byte array from a null-terminated C-string. This constructor allows implicit conversion.</p>
 <p><span class="ensures" /></p>
 <ul>
  <li><code>data() == cstr</code></li>
  <li><code>size() == std::strlen(cstr)</code></li>
 </ul>
</div>

<div class="reference-more">
 <h4 id="bytes-bytes-str"><code>Bytes::Bytes(const std::string & str)</code></h4>
 <p>Creates a byte array from a standard string. This constructor allows implicit conversion.</p>
 <p><span class="ensures" /></p>
 <ul>
  <li><code>data() == str.data()</code></li>
  <li><code>size() == str.size()</code></li>
 </ul>
</div>

<div class="reference-more">
 <h4 id="bytes-bytes-data-size"><code>Bytes::Bytes(const void * data, size_t size)</code></h4>
 <p>Creates a byte array from arbitrary data.</p>
 <p><span class="ensures" /></p>
 <ul>
  <li><code>data() == data</code></li>
  <li><code>size() == size</code></li>
 </ul>
</div>

<div class="reference-more">
 <h4 id="bytes-empty"><code>bool Bytes::empty() const</code></h4>
 <p>Tells whether the byte array is empty. Returns <code>true</code> if the number of bytes is zero, <code>false</code> otherwise.</p>
</div>

<div class="reference-more">
 <h4 id="bytes-clear"><code>void Bytes::clear()</code></h4>
 <p>Let this byte array point to an empty array.</p>
 <p><span class="ensures" /></p>
 <ul>
  <li><code>data() != nullptr</code></li>
  <li><code>size() == 0</code></li>
 </ul>
</div>

<div class="reference-more">
 <h4 id="bytes-tostring"><code>std::string toString() const</code></h4>
 <p>Returns a string which contains a copy of the wrapped data. <code>std::string</code> is used as a convenient byte buffer and may contain bytes that are not printable or even null-bytes.</p>
 <p><span class="ensures" /></p>
 <ul>
  <li><code>data() != return_value.data()</code></li>
  <li><code>size() == return_value.size()</code></li>
 </ul>
</div>

<div class="reference-more">
 <h4 id="operator-eq"><code>bool operator==(const Bytes & lhs, const Bytes & rhs)</code></h4>
 <p>Compares two byte arrays for equality. Two byte arrays are equal, if they wrap the same number of bytes, and which are equal after byte-wise comparison.</p>
</div>


## callables.hpp

```cpp
#include <multimap/callables.hpp>
namespace multimap
```

This file contains some convenient function objects implementing the [Predicate](#interfaces-predicate) interface.

### struct Contains

A function object that checks if a byte array contains another one in terms of a substring. Note that the empty byte array is a substring of any other byte array.

<table class="reference-table">
<tbody>
 <tr>
  <th colspan="2">Member functions</th>
 </tr>
 <tr>
  <td><code>explicit</code></td>
  <td>
   <code>Contains(const Bytes & bytes)</code>
   <div>Creates a functor that checks for containment of bytes when called.<div>
  </td>
 </tr>
 <tr>
  <td><code>const Bytes &</code></td>
  <td>
   <code>bytes() const</code>
   <div>Returns a read-only reference to the wrapped byte array.<div>
  </td>
 </tr>
 <tr>
  <td><code>bool</code></td>
  <td>
   <code>operator()(const Bytes & bytes) const</code>
   <div>Returns true if bytes contains the wrapped byte array, false otherwise.<div>
  </td>
 </tr>
</tbody>
</table>

### struct StartsWith

A function object that checks if a byte array has a certain prefix.

<table class="reference-table">
<tbody>
 <tr>
  <th colspan="2">Member functions</th>
 </tr>
 <tr>
  <td><code>explicit</code></td>
  <td>
   <code>StartsWith(const Bytes & bytes)</code>
   <div>Creates a functor that checks if a byte array starts with bytes when called.<div>
  </td>
 </tr>
 <tr>
  <td><code>const Bytes &</code></td>
  <td>
   <code>bytes() const</code>
   <div>Returns a read-only reference to the wrapped byte array.<div>
  </td>
 </tr>
 <tr>
  <td><code>bool</code></td>
  <td>
   <code>operator()(const Bytes & bytes) const</code>
   <div>Returns true if bytes starts with the wrapped byte array, false otherwise.<div>
  </td>
 </tr>
</tbody>
</table>

### struct EndsWith

A function object that checks if a byte array has a certain suffix.

<table class="reference-table">
<tbody>
 <tr>
  <th colspan="2">Member functions</th>
 </tr>
 <tr>
  <td><code>explicit</code></td>
  <td>
   <code>EndsWith(const Bytes & bytes)</code>
   <div>Creates a functor that checks if a byte array ends with bytes when called.<div>
  </td>
 </tr>
 <tr>
  <td><code>const Bytes &</code></td>
  <td>
   <code>bytes() const</code>
   <div>Returns a read-only reference to the wrapped byte array.<div>
  </td>
 </tr>
 <tr>
  <td><code>bool</code></td>
  <td>
   <code>operator()(const Bytes & bytes) const</code>
   <div>Returns true if bytes ends with the wrapped byte array, false otherwise.<div>
  </td>
 </tr>
</tbody>
</table>


## Map.hpp

```cpp
#include <multimap/Map.hpp>
namespace multimap
```

### class Map

This class implements a 1:n key-value store where each key is associated with a list of values. When putting a key-value pair into the map, the value is appended to the end of the list that is associated with the key. If no such key-list pair already exist, it will be created. Looking up a key returns a read-only iterator for the associated list. If the key does not exist, the list is considered to be empty and the returned iterator has no values to deliver. From a user's point of view there is no distinction between an empty list and a non-existing list.

Map also supports removing or replacing values. When a value is removed, it will be marked as such for the moment making it invisible for subsequent iterations. Running an [optimze](#map-optimize) operation removes the data physically. The replace operation is implemented as a remove of the old value followed by an insert/put of the new value. In other words, the replacement is not in-place, but the new value is always the last value in the corresponding list. To restore a certain order an [optimze](#map-optimize) operation can be run as well. However, optimization is considered a less frequent, more administrative task.

The class is designed to be a fast and mutable 1:n key-value store. For that reason, Map holds the entire key set in memory. This is also true for keys that were removed from the map at runtime. In addition, each key is associated with a write buffer which altogether contribute a lot to the total memory footprint of the map. Therefore, the number of keys to be put is limited by the amount of available memory. However, this simple design was intended in favour of performance, especially due to the fact that memory is relatively cheap and (server) machines are equipped with more and more of it. As a rule of thumb, assuming keys with 10 bytes in size on average, Map scales up to 10 M keys on desktop machines with 8 GiB of RAM and up to 100 M keys on server machines with 64 GiB of RAM. The number of values, though, is practically limited only by the amount of disk space. For more information, please visit the [overview](overview/#block-organization) page.

<table class="reference-table">
<tbody>
 <tr>
  <th colspan="2">Member types</th>
 </tr>
 <tr>
  <td></td>
  <td>
   <code>Limits</code>
   <div>Provides static methods to ask for maximum key and value sizes. <a href="#type-maplimits">more...</a><div>
  </td>
 </tr>
 <tr>
  <td></td>
  <td>
   <code>Stats</code>
   <div>Type that reports statistical information about an instance of Map. <a href="#type-mapstats">more...</a><div>
  </td>
 </tr>
 <tr>
  <td></td>
  <td>
   <code>Iterator</code>
   <div>Input iterator type that reads a list of values. <a href="#type-mapiterator">more...</a><div>
  </td>
 </tr>
 <tr>
  <th colspan="2">Member functions</th>
 </tr>
 <tr>
  <td><code>explicit</code></td>
  <td>
   <code>Map(const boost::filesystem::path & directory)</code>
   <div>Opens an already existing map located in directory. <a href="#map-map-directory">more...</a><div>
  </td>
 </tr>
 <tr>
  <td></td>
  <td>
   <code>Map(const boost::filesystem::path & directory,</code><br>
   <code><script>nbsp(4)</script>const Options & options)</code>
   <div>Opens or creates a map in directory. <a href="#map-map-directory-options">more...</a><div>
  </td>
 </tr>
 <tr>
  <td></td>
  <td>
   <code>~Map()</code>
   <div>Flushes all buffered data to disk, closes the map, and unlocks the directory where the map is located.<div>
  </td>
 </tr>
 <tr>
  <td><code>void</code></td>
  <td>
   <code>put(const Bytes & key, const Bytes & value)</code>
   <div>Appends value to the end of the list associated with key. <a href="#map-put">more...</a><div>
  </td>
 </tr>
 <tr>
  <td><code>Iterator</code></td>
  <td>
   <code>get(const Bytes & key) const</code>
   <div>Returns a read-only iterator for the list associated with key. <a href="#map-get">more...</a><div>
  </td>
 </tr>
 <tr>
  <td><code>bool</code></td>
  <td>
   <code>removeKey(const Bytes & key)</code>
   <div>Removes all values associated with key. <a href="#map-remove-key">more...</a><div>
  </td>
 </tr>
 <tr>
  <td>
    <code>template</code><br>
    <code>uint32_t</code>
  </td>
  <td>
   <code>&lt;typename Predicate&gt;</code><br>
   <code>removeKeys(Predicate predicate)</code>
   <div>Removes all values associated with keys for which predicate yields true. <a href="#map-remove-keys">more...</a><div>
  </td>
 </tr>
 <tr>
  <td>
    <code>template</code><br>
    <code>bool</code>
  </td>
  <td>
   <code>&lt;typename Predicate&gt;</code><br>
   <code>removeValue(const Bytes & key, Predicate predicate)</code>
   <div>Removes the first value from the list associated with key for which predicate yields true. <a href="#map-remove-value">more...</a><div>
  </td>
 </tr>
 <tr>
  <td>
    <code>template</code><br>
    <code>uint32_t</code>
  </td>
  <td>
   <code>&lt;typename Predicate&gt;</code><br>
   <code>removeValues(const Bytes & key, Predicate predicate)</code>
   <div>Removes all values from the list associated with key for which predicate yields true. <a href="#map-remove-values">more...</a><div>
  </td>
 </tr>
 <tr>
  <td>
    <code>template</code><br>
    <code>bool</code>
  </td>
  <td>
   <code>&lt;typename Function&gt;</code><br>
   <code>replaceValue(const Bytes & key, Function map)</code>
   <div>Replaces the first value in the list associated with key by the result of invoking map. Values for which map returns the empty string are not replaced. <a href="#map-replace-value">more...</a><div>
  </td>
 </tr>
 <tr>
  <td>
    <code>template</code><br>
    <code>uint32_t</code>
  </td>
  <td>
   <code>&lt;typename Function&gt;</code><br>
   <code>replaceValues(const Bytes & key, Function map)</code>
   <div>Replaces each value in the list associated with key by the result of invoking map. Values for which map returns the empty string are not replaced. <a href="#map-replace-values">more...</a><div>
  </td>
 </tr>
 <tr>
  <td><code>uint32_t</code></td>
  <td>
   <code>replaceValues(const Bytes & key,</code><br>
   <code><script>nbsp(14)</script>const Bytes & old_value,</code><br>
   <code><script>nbsp(14)</script>const Bytes & new_value)</code>
   <div>Replaces each value in the list associated with key which is equal to old_value by new_value. <a href="#map-replace-values-old-new">more...</a><div>
  </td>
 </tr>
 <tr>
  <td>
    <code>template</code><br>
    <code>void</code>
  </td>
  <td>
   <code>&lt;typename Procedure&gt;</code><br>
   <code>forEachKey(Procedure process) const</code>
   <div>Applies process to each key whose list is not empty. <a href="#map-for-each-key">more...</a><div>
  </td>
 </tr>
 <tr>
  <td>
    <code>template</code><br>
    <code>void</code>
  </td>
  <td>
   <code>&lt;typename Procedure&gt;</code><br>
   <code>forEachValue(const Bytes & key, Procedure process) const</code>
   <div>Applies process to each value associated with key. <a href="#map-for-each-value">more...</a><div>
  </td>
 </tr>
 <tr>
  <td>
    <code>template</code><br>
    <code>void</code>
  </td>
  <td>
   <code>&lt;typename BinaryProcedure&gt;</code><br>
   <code>forEachEntry(BinaryProcedure process) const</code>
   <div>Applies process to each key-iterator pair. <a href="#map-for-each-entry">more...</a><div>
  </td>
 </tr>
 <tr>
  <td>
    <code>size_t</code>
  </td>
  <td>
   <code>getNumPartitions() const</code>
   <div>Returns the number of partitions. This value could be different from that specified in options when creating the map due to the fact that the next prime number has been taken.<div>
  </td>
 </tr>
 <tr>
 <tr>
  <td>
    <code>std::vector&lt;Stats&gt;</code>
  </td>
  <td>
   <code>getStats() const</code>
   <div>Returns statistical information about each partition of the map. <a href="#map-get-stats">more...</a><div>
  </td>
 </tr>
 <tr>
  <td>
    <code>Stats</code>
  </td>
  <td>
   <code>getTotalStats() const</code>
   <div>Returns statistical information about the map. <a href="#map-get-total-stats">more...</a><div>
  </td>
 </tr>
 <tr>
  <td>
    <code>bool</code>
  </td>
  <td>
   <code>isReadOnly() const</code>
   <div>Returns true if the map is read-only, false otherwise.<div>
  </td>
 </tr>
 <tr>
  <th colspan="2">Static member functions</th>
 </tr>
 <tr>
  <td>
    <code>std::vector&lt;Stats&gt;</code>
  </td>
  <td>
   <code>stats(const boost::filesystem::path & directory)</code>
   <div>Returns statistical information about each partition of the map located in directory. <a href="#map-stats">more...</a><div>
  </td>
 </tr>
 <tr>
  <td>
    <code>void</code>
  </td>
  <td>
   <code>importFromBase64(const boost::filesystem::path & directory,</code><br>
   <code><script>nbsp(17)</script>const boost::filesystem::path & input)</code>
   <div>Imports key-value pairs from an input file or directory into the map located in directory. <a href="#map-import-from-base64">more...</a><div>
  </td>
 </tr>
 <tr>
  <td>
    <code>void</code>
  </td>
  <td>
   <code>importFromBase64(const boost::filesystem::path & directory,</code><br>
   <code><script>nbsp(17)</script>const boost::filesystem::path & input)</code><br>
   <code><script>nbsp(17)</script>const Options & options)</code>
   <div>Same as before, but gives the user more control by providing an <a href="#class-options">Options</a> parameter which is passed to the constructor of Map when opening. This way a map can be created if it does not already exist. <a href="#map-map-directory-options">more...</a><div>
  </td>
 </tr>
 <tr>
  <td>
    <code>void</code>
  </td>
  <td>
   <code>exportToBase64(const boost::filesystem::path & directory,</code><br>
   <code><script>nbsp(15)</script>const boost::filesystem::path & output)</code>
   <div>Exports all key-value pairs from the map located in directory to a file denoted by output. <a href="#map-export-to-base64">more...</a><div>
  </td>
 </tr>
 <tr>
  <td>
    <code>void</code>
  </td>
  <td>
   <code>exportToBase64(const boost::filesystem::path & directory,</code><br>
   <code><script>nbsp(15)</script>const boost::filesystem::path & output,</code><br>
   <code><script>nbsp(15)</script>const Options & options)</code>
   <div>Same as before, but gives the user more control by providing an <a href="#class-options">Options</a> parameter. Most users will use this to pass a compare function that triggers a sorting of all lists before exporting them.<div>
  </td>
 </tr>
 <tr>
  <td>
    <code>void</code>
  </td>
  <td>
   <code>optimize(const boost::filesystem::path & directory,</code><br>
   <code><script>nbsp(9)</script>const boost::filesystem::path & output)</code>
   <div>Rewrites the map located in directory to the directory denoted by output performing various optimizations. <a href="#map-optimize">more...</a><div>
  </td>
 </tr>
 <tr>
  <td>
    <code>void</code>
  </td>
  <td>
   <code>optimize(const boost::filesystem::path & directory,</code><br>
   <code><script>nbsp(9)</script>const boost::filesystem::path & output,</code><br>
   <code><script>nbsp(9)</script>const Options & options)</code>
   <div>Same as before, but gives the user more control by providing an <a href="#class-options">Options</a> parameter. Most users will use this to pass a compare function that triggers a sorting of all lists.<div>
  </td>
 </tr>
</tbody>
</table>

<div class="reference-more">
 <h4 id="map-map-directory"><code>explicit Map::Map(const boost::filesystem::path & directory)</code></h4>
 <p>Opens an already existing map located in directory.</p>
 <p><span class="acquires" />a <a href="#directory-lock">directory lock</a> on directory.</p>
 <p><span class="throws" /><a href="http://en.cppreference.com/w/cpp/error/runtime_error" target="_blank">std::runtime_error</a> if one of the following is true:</p>
 <ul>
  <li>the directory does not exist</li>
  <li>the directory cannot be locked</li>
  <li>the directory does not contain a map</li>
 </ul>
</div>

<div class="reference-more">
 <h4 id="map-map-directory-options">
  <code>Map::Map(const boost::filesystem::path & directory,</code><br>
  <code><script>nbsp(9)</script>const Options & options)</code>
 </h4>
 <p>Opens or creates a map in directory. For the latter, you need to set <code>options.create_if_missing = true</code>. If an error should be raised in case the map already exists, set <code>options.error_if_exists = true</code>. When a new map is created other fields in options are used to configure the map's block size and number of partitions. See <a href="#optionshpp">Options</a> for more information.</p>
 <p><span class="acquires" />a <a href="#directory-lock">directory lock</a> on directory.</p>
 <p><span class="throws" /><a href="http://en.cppreference.com/w/cpp/error/runtime_error" target="_blank">std::runtime_error</a> if one of the following is true:</p>
 <ul>
  <li>the directory does not exist</li>
  <li>the directory cannot be locked</li>
 </ul>
 <p>when <code>options.create_if_missing = false</code> (which is the default)</p>
 <ul>
  <li>the directory does not contain a map</li>
 </ul>
 <p>when <code>options.create_if_missing = true</code> and no map exists</p>
 <ul>
  <li><code>options.block_size</code> is zero</li>
  <li><code>options.block_size</code> is not a power of two</li>
  <li><code>options.buffer_size</code> is not a multiple of the block size</li>
 </ul>
 <p>when <code>options.error_if_exists = true</code></p>
 <ul>
  <li>the directory already contains a map</li>
 </ul>
</div>

<div class="reference-more">
 <h4 id="map-put"><code>void Map::put(const Bytes & key, const Bytes & value)</code></h4>
 <p>Appends value to the end of the list associated with key.</p>
 <p><span class="acquires" /></p>
 <ul>
  <li>a <a href="#writer-lock">writer lock</a> on the map object.</li>
  <li>a <a href="#writer-lock">writer lock</a> on the list associated with key.</li>
 </ul>
 <p><span class="throws"/><a href="http://en.cppreference.com/w/cpp/error/runtime_error" target="_blank">std::runtime_error</a> if one of the following is true:</p>
 <ul>
  <li><code>key.size() > Map::Limits::maxKeySize()</code></li>
  <li><code>value.size() > Map::Limits::maxValueSize()</code></li>
  <li>the map was opened in read-only mode</li>
 </ul>
</div>

<div class="reference-more">
 <h4 id="map-get"><code>Map::Iterator Map::get(const Bytes & key) const</code></h4>
 <p>Returns a read-only iterator for the list associated with key. If the key does not exist, an empty iterator that has no values is returned. A non-empty iterator owns a lock on the associated list that is released automatically when the lifetime of the iterator ends. Note that objects of class <a href="#map-iterator">Map::Iterator</a> are moveable.</p>
 <p><span class="acquires" /></p>
 <ul>
  <li>a <a href="#reader-lock">reader lock</a> on the map object.</li>
  <li>a <a href="#reader-lock">reader lock</a> on the list associated with key.</li>
 </ul>
</div>

<div class="reference-more">
 <h4 id="map-remove-key"><code>bool Map::removeKey(const Bytes & key)</code></h4>
 <p>Removes all values associated with key.</p>
 <p><span class="acquires" /></p>
 <ul>
  <li>a <a href="#reader-lock">reader lock</a> on the map object.</li>
  <li>a <a href="#writer-lock">writer lock</a> on the list associated with key.</li>
 </ul>
 <p><span class="returns" />true if any values have been removed, false otherwise.</p>
</div>

<div class="reference-more">
 <h4 id="map-remove-keys">
  <code>template &lt;typename Predicate&gt;</code><br>
  <code>uint32_t Map::removeKeys(Predicate predicate)</code>
 </h4>
 <p>Removes all values associated with keys for which predicate yields true. The predicate can be any callable that implements the <a href="#predicate">Predicate</a> interface.</p>
 <p><span class="acquires" /></p>
 <ul>
  <li>a <a href="#reader-lock">reader lock</a> on the map object.</li>
  <li>a <a href="#writer-lock">writer lock</a> on lists associated with matching keys.</li>
 </ul>
 <p><span class="returns" />the number of keys for which any values have been removed.</p>
</div>

<div class="reference-more">
 <h4 id="map-remove-value">
  <code>template &lt;typename Predicate&gt;</code><br>
  <code>bool Map::removeValue(const Bytes & key, Predicate predicate)</code>
 </h4>
 <p>Removes the first value from the list associated with key for which predicate yields true. The predicate can be any callable that implements the <a href="#predicate">Predicate</a> interface.</p>
 <p><span class="acquires" /></p>
 <ul>
  <li>a <a href="#reader-lock">reader lock</a> on the map object.</li>
  <li>a <a href="#writer-lock">writer lock</a> on the list associated with key.</li>
 </ul>
 <p><span class="returns" />true if any value has been removed, false otherwise.</p>
</div>

<div class="reference-more">
 <h4 id="map-remove-values">
  <code>template &lt;typename Predicate&gt;</code><br>
  <code>uint32_t Map::removeValues(const Bytes & key, Predicate predicate)</code>
 </h4>
 <p>Removes all values from the list associated with key for which predicate yields true. The predicate can be any callable that implements the <a href="#predicate">Predicate</a> interface.</p>
 <p><span class="acquires" /></p>
 <ul>
  <li>a <a href="#reader-lock">reader lock</a> on the map object.</li>
  <li>a <a href="#writer-lock">writer lock</a> on the list associated with key.</li>
 </ul>
 <p><span class="returns" />the number of values removed.</p>
</div>

<div class="reference-more">
 <h4 id="map-replace-value">
  <code>template &lt;typename Function&gt;</code><br>
  <code>bool Map::replaceValue(const Bytes & key, Function map)</code>
 </h4>
 <p>Replaces the first value in the list associated with key by the result of invoking map. Values for which map returns the empty string are not replaced. The map function can be any callable that implements the <a href="#function">Function</a> interface.</p>
 <p>Note that a replace operation is actually implemented in terms of a remove of the old value followed by an insert/put of the new value. Thus the new value is always the last value in the list. In other words, the replacement is not in-place.</p>
 <p><span class="acquires" /></p>
 <ul>
  <li>a <a href="#reader-lock">reader lock</a> on the map object.</li>
  <li>a <a href="#writer-lock">writer lock</a> on the list associated with key.</li>
 </ul>
 <p><span class="returns" />true if any value has been replaced, false otherwise.</p>
</div>

<div class="reference-more">
 <h4 id="map-replace-values">
  <code>template &lt;typename Function&gt;</code><br>
  <code>uint32_t Map::replaceValues(const Bytes & key, Function map)</code>
 </h4>
 <p>Replaces each value in the list associated with key by the result of invoking map. Values for which map returns the empty string are not replaced. The map function can be any callable that implements the <a href="#function">Function</a> interface.</p>
 <p>Note that a replace operation is actually implemented in terms of a remove of the old value followed by an insert/put of the new value. Thus the new value is always the last value in the list. In other words, the replacement is not in-place.</p>
 <p><span class="acquires" /></p>
 <ul>
  <li>a <a href="#reader-lock">reader lock</a> on the map object.</li>
  <li>a <a href="#writer-lock">writer lock</a> on the list associated with key.</li>
 </ul>
 <p><span class="returns" />the number of values replaced.</p>
</div>

<div class="reference-more">
 <h4 id="map-replace-values-old-new">
  <code>uint32_t Map::replaceValues(const Bytes & key, </code><br>
  <code><script>nbsp(28)</script>const Bytes & old_value,</code><br>
  <code><script>nbsp(28)</script>const Bytes & new_value)</code>
 </h4>
 <p>Replaces each value in the list associated with key which is equal to old_value by new_value.</p>
 <p>Note that a replace operation is actually implemented in terms of a remove of the old value followed by an insert/put of the new value. Thus the new value is always the last value in the list. In other words, the replacement is not in-place.</p>
 <p><span class="acquires" /></p>
 <ul>
  <li>a <a href="#reader-lock">reader lock</a> on the map object.</li>
  <li>a <a href="#writer-lock">writer lock</a> on the list associated with key.</li>
 </ul>
 <p><span class="returns" />the number of values replaced.</p>
</div>

<div class="reference-more">
 <h4 id="map-for-each-key">
  <code>template &lt;typename Procedure&gt;</code><br>
  <code>void Map::forEachKey(Procedure process) const</code>
 </h4>
 <p>Applies process to each key whose list is not empty. The process argument can be any callable that implements the <a href="#procedure">Procedure</a> interface. </p>
 <p><span class="acquires" />a <a href="#reader-lock">reader lock</a> on the map object.</p>
 <p><span class="returns" />the number of values replaced.</p>
</div>

<div class="reference-more">
 <h4 id="map-for-each-value">
  <code>template &lt;typename Procedure&gt;</code><br>
  <code>void Map::forEachValue(const Bytes & key, Procedure process) const</code>
 </h4>
 <p>Applies process to each value associated with key. The process argument can be any callable that implements the <a href="#procedure">Procedure</a> interface. </p>
 <p><span class="acquires" /></p>
 <ul>
  <li>a <a href="#reader-lock">reader lock</a> on the map object.</li>
  <li>a <a href="#reader-lock">reader lock</a> on the list associated with key.</li>
 </ul>
</div>

<div class="reference-more">
 <h4 id="map-for-each-entry">
  <code>template &lt;typename BinaryProcedure&gt;</code><br>
  <code>void Map::forEachEntry(BinaryProcedure process) const</code>
 </h4>
 <p>Applies process to each key-iterator pair. The process argument can be any callable that implements the <a href="#binaryprocedure">BinaryProcedure</a> interface. </p>
 <p><span class="acquires" /></p>
 <ul>
  <li>a <a href="#reader-lock">reader lock</a> on the map object.</li>
  <li>a <a href="#reader-lock">reader lock</a> on the list that is currently processed.</li>
 </ul>
</div>

<div class="reference-more">
 <h4 id="map-get-stats">
  <code>std::vector&lt;<a href="#class-map-stats">Map::Stats</a>&gt; Map::getStats() const</code>
 </h4>
 <p>Returns statistical information about each partition of the map. This operation requires a traversal of the entire map visiting each entry.</p>
 <p><span class="acquires" /></p>
 <ul>
  <li>a <a href="#reader-lock">reader lock</a> on the map object.</li>
  <li>a <a href="#reader-lock">reader lock</a> on the list that is currently visited.</li>
 </ul>
</div>

<div class="reference-more">
 <h4 id="map-get-total-stats">
  <code><a href="#class-map-stats">Map::Stats</a> Map::getTotalStats() const</code>
 </h4>
 <p>Returns statistical information about the map. In fact, this method computes the total values from the result returned by calling the previous method.</p>
 <p><span class="acquires" /></p>
 <ul>
  <li>a <a href="#reader-lock">reader lock</a> on the map object.</li>
  <li>a <a href="#reader-lock">reader lock</a> on the list that is currently visited.</li>
 </ul>
</div>

<div class="reference-more">
 <h4 id="map-stats">
  <code>static std::vector&lt;<a href="#class-map-stats">Map::Stats</a>&gt; Map::stats(</code><br>
  <code><script>nbsp(8)</script>const boost::filesystem::path & directory)</code>
 </h4>
 <p>Returns statistical information about each partition of the map located in directory. This method is similar to <a href="#map-get-stats">Map::getStats()</a> except that the map does not need to be instanciated.</p>
 <p><span class="acquires" />a <a href="#directory-lock">directory lock</a> on directory.</p>
 <p><span class="throws"/> everything thrown by the constructor of class <a href="#class-map">Map</a>.</p>
</div>

<div class="reference-more">
 <h4 id="map-import-from-base64">
  <code>static void Map::importFromBase64(</code><br>
  <code><script>nbsp(8)</script>const boost::filesystem::path & directory,</code><br>
  <code><script>nbsp(8)</script>const boost::filesystem::path & input)</code>
 </h4>
 <p>Imports key-value pairs from an input file or directory into the map located in directory. If input refers to a directory all files in that directory will be imported, except hidden files starting with a dot and other sub-directories. A description of the file format can be found in the <a href="/overview/#multimap-import">overview</a> section.</p>
 <p><span class="acquires" />a <a href="#directory-lock">directory lock</a> on directory.</p>
 <p><span class="throws"/> everything thrown by the constructor of class <a href="#class-map">Map</a> or <a href="http://en.cppreference.com/w/cpp/error/runtime_error" target="_blank">std::runtime_error</a> if the input file or directory cannot be read.</p>
</div>

<div class="reference-more">
 <h4 id="map-export-to-base64">
  <code>static void Map::exportToBase64(</code><br>
  <code><script>nbsp(8)</script>const boost::filesystem::path & directory,</code><br>
  <code><script>nbsp(8)</script>const boost::filesystem::path & output)</code>
 </h4>
 <p>Exports all key-value pairs from the map located in directory to a file denoted by output. If the file already exists, its content will be overwritten. The generated file is in canonical form as described in the <a href="/overview/#multimap-export">overview</a> section.</p>
 <p><span class="acquires" />a <a href="#directory-lock">directory lock</a> on directory.</p>
 <p><span class="throws" /><a href="http://en.cppreference.com/w/cpp/error/runtime_error" target="_blank">std::runtime_error</a> if one of the following is true:</p>
 <ul>
  <li>the directory does not exist</li>
  <li>the directory cannot be locked</li>
  <li>the directory does not contain a map</li>
  <li>the creation of the output file failed</li>
 </ul>
</div>

<div class="reference-more">
 <h4 id="map-optimize">
  <code>static void Map::optimize(</code><br>
  <code><script>nbsp(8)</script>const boost::filesystem::path & directory,</code><br>
  <code><script>nbsp(8)</script>const boost::filesystem::path & output)</code>
 </h4>
 <p>Rewrites the map located in directory to the directory denoted by output performing various optimizations. For more details please refer to the <a href="/overview/#multimap-optimize">overview</a> section.</p>
 <p><span class="acquires" />a <a href="#directory-lock">directory lock</a> on directory.</p>
 <p><span class="throws" /><a href="http://en.cppreference.com/w/cpp/error/runtime_error" target="_blank">std::runtime_error</a> if one of the following is true:</p>
 <ul>
  <li>the directory does not exist</li>
  <li>the directory cannot be locked</li>
  <li>the directory does not contain a map</li>
  <li>the creation of a new map in output failed</li>
 </ul>
</div>

### type Map::Limits

This type represents a namespace that provides static methods for obtaining system limitations. Those limits which define constraints on user supplied data also serve as preconditions.

<table class="reference-table">
<tbody>
 <tr>
  <th colspan="2">Static member functions</th>
 </tr>
 <tr>
  <td><code>uint32_t</code></td>
  <td>
   <code>maxKeySize()</code>
   <div>Returns the maximum size in number of bytes for a key to put.<div>
  </td>
 </tr>
 <tr>
  <td><code>uint32_t</code></td>
  <td>
   <code>maxValueSize()</code>
   <div>Returns the maximum size in number of bytes for a value to put.<div>
  </td>
 </tr>
</tbody>
</table>

### type Map::Stats

This type is a pure data holder for reporting statistical information about an instance of <a href="#class-map">Map</a>.

<table class="reference-table">
<tbody>
 <tr>
  <th colspan="2">Data members</th>
 </tr>
 <tr>
  <td><code>uint64_t</code></td>
  <td>
   <code>block_size</code>
   <div>Tells the block size defined in <a href="#struct-options">Options</a> when creating the map.<div>
  </td>
 </tr>
 <tr>
  <td><code>uint64_t</code></td>
  <td>
   <code>key_size_avg</code>
   <div>Tells the average size in number of bytes of all keys. Note that keys that are currently not associated with any value are not taken into account.<div>
  </td>
 </tr>
</tbody>
</table>

## Interfaces

The interfaces described here are requirements expected by some user-provided function objects. They are typically employed as template parameters and are not to be confused with abstract classes used in object-oriented programming. Sometimes this type of interfaces is also referred to as [concepts](http://en.cppreference.com/w/cpp/concept). Note that [lambda functions](http://en.cppreference.com/w/cpp/language/lambda) are unnamed function objects.

### Compare

A function object that is applied to two instances of class [Bytes](#class-bytes) returning a boolean that tells if the left operand is less than the right operand. This interface is equivalent to the Compare concept described [here](http://en.cppreference.com/w/cpp/concept/Compare). Objects implementing this interface are used in sorting operations.

<table class="reference-table">
<tbody>
 <tr>
  <th colspan="2">Required member function</th>
 </tr>
 <tr>
  <td><code>bool</code></td>
  <td>
   <code>operator()(const Bytes & lhs, const Bytes & rhs) const</code>
   <div>Returns true if lhs is considered less than rhs, false otherwise.<div>
  </td>
 </tr>
</tbody>
</table>

### Function

A function is a function object that is applied to an instance of class [Bytes](#class-bytes) returning a [std::string](http://en.cppreference.com/w/cpp/string/basic_string). The returned string serves as a managed byte buffer and may contain arbitrary data. Functions are used to map input values to output values.

<table class="reference-table">
<tbody>
 <tr>
  <th colspan="2">Required member function</th>
 </tr>
 <tr>
  <td><code>std::string</code></td>
  <td>
   <code>operator()(const Bytes & bytes) const</code>
   <div>Maps the given byte array to another byte array returned as string.<div>
  </td>
 </tr>
</tbody>
</table>

### Predicate

A predicate is a function object that is applied to an instance of class [Bytes](#class-bytes) returning a boolean value. Predicates are used to select keys or values for some further operation.

<table class="reference-table">
<tbody>
 <tr>
  <th colspan="2">Required member function</th>
 </tr>
 <tr>
  <td><code>bool</code></td>
  <td>
   <code>operator()(const Bytes & bytes) const</code>
   <div>Returns a boolean value after evaluating the given byte array.<div>
  </td>
 </tr>
</tbody>
</table>

### Procedure

A procedure is a function object that is applied to an instance of class [Bytes](#class-bytes) without returning a value. Procedures may have a state that changes during application. They are used to operate on keys or values, e.g. to collect information about them.

<table class="reference-table">
<tbody>
 <tr>
  <th colspan="2">Required member function</th>
 </tr>
 <tr>
  <td><code>void</code></td>
  <td>
   <code>operator()(const Bytes & bytes)</code>
   <div>Processes the given byte array, possibly changing the functor's state.<div>
  </td>
 </tr>
</tbody>
</table>

### BinaryProcedure

A binary procedure is a function object that is applied to a pair of objects without returning a value. The first object being an instance of class [Bytes](#class-bytes) and the second object being an instance of class [Map::Iterator](#class-map-iterator). Binary procedures may have a state that changes during application. They are used to operate on key-iterator pairs when traversing a map.

<table class="reference-table">
<tbody>
 <tr>
  <th colspan="2">Required member function</th>
 </tr>
 <tr>
  <td><code>void</code></td>
  <td>
   <code>operator()(const Bytes & key, Map::Iterator iterator)</code>
   <div>Processes a list iterator that is associated with key, possibly changing the functor's state. Note that the iterator will be moved, so that the callable becomes the owner of the iterator. Also note that as long as an iterator is alive, the corresponding list is locked by a <a href="#reader-lock">reader lock</a>.<div>
  </td>
 </tr>
</tbody>
</table>


## Locking

### Reader Lock

### Writer Lock

This operation requires exclusive access to the associated list and therefore tries to acquire a writer lock for it. The call will block if the list is already locked, either by another writer lock or by one or more reader locks, until all of these locks are released.

## class Iterator<bool>

```cpp
#include <multimap/Iterator.hpp>
namespace multimap
```

This class template implements a forward iterator on a list of values. The template parameter decides whether an instantiation can delete values from the underlying list while iterating or not. If the actual parameter is `true` a const iterator will be created where the `DeleteValue()` method is disabled, and vice versa.

The iterator supports lazy initialization, which means that no IO operation is performed until one of the methods `SeekToFirst()`, `SeekTo(target)`, or `SeekTo(predicate)` is called. This might be useful in cases where multiple iterators have to be requested first to determine in which order they have to be processed.

The iterator also owns a lock for the underlying list to synchronize concurrent access. There are two types of locks: a reader lock (also called shared lock) and a writer lock (also called unique or exclusive lock). The former will be owned by a read-only iterator aka `Iterator<true>`, the latter will be owned by a read-write iterator aka `Iterator<false>`. The lock is automatically released when the lifetime of an iterator object ends and its destructor is called.

Users normally don't need to include or instantiate this class directly, but use the typedefs [`Map::ListIter`](#map-listiter) and [`Map::ConstListIter`](#map-constlistiter) instead.

Members       |
-------------:|-----------------------------------------------------------------
              | [`Iterator()`](#iterator-iterator)
`std::size_t` | [`NumValues() const`](#iterator-numvalues)
`void`        | [`SeekToFirst()`](#iterator-seektofirst)
`void`        | [`SeekTo(const Bytes& target)`](#iterator-seektotarget)
`void`        | [`SeekTo(Callables::Predicate predicate)`](#iterator-seektopredicate)
`bool`        | [`HasValue() const`](#iterator-hasvalue)
`Bytes`       | [`GetValue() const`](#iterator-getvalue)
`void`        | [`DeleteValue()`](#iterator-deletevalue)
`void`        | [`Next()`](#iterator-next)

<span class='declaration' id='iterator-iterator'>`Iterator()`</span>

Creates a default instance that has no values to iterate.

Postconditions:

* `NumValues() == 0`

<span class='declaration' id='iterator-numvalues'>
 `std::size_t NumValues() const`
</span>

Returns the total number of values to iterate. This number does not change when the iterator moves forward. The method may be called at any time, even if `SeekToFirst()` or one of its friends have not been called.

<span class='declaration' id='iterator-seektofirst'>
 `void SeekToFirst()`
</span>

Initializes the iterator to point to the first value, if any. This process will trigger disk IO if necessary. The method can also be used to seek back to the beginning of the list at the end of an iteration.

<span class='declaration' id='iterator-seektotarget'>
 `void SeekTo(const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& target)`
</span>

Initializes the iterator to point to the first value in the list that is equal to `target`, if any. This process will trigger disk IO if necessary.
   
<span class='declaration' id='iterator-seektopredicate'>
 `void SeekTo(`[`Callables::Predicate`](#callables-predicate)&nbsp;&nbsp;`predicate)`
</span>

Initializes the iterator to point to the first value for which `predicate` yields `true`, if any. This process will trigger disk IO if necessary.
   
<span class='declaration' id='iterator-hasvalue'>
 `bool HasValue() const`
</span>

Tells whether the iterator points to a value. If the result is `true`, the iterator may be dereferenced via `GetValue()`.
   
<span class='declaration' id='iterator-getvalue'>
 [`Bytes`](#class-bytes)&nbsp;&nbsp;`GetValue() const`
</span>

Returns the current value. The returned [`Bytes`](#class-bytes) object wraps a pointer to data that is managed by the iterator. Hence, this pointer is only valid as long as the iterator does not move forward. Therefore, the value should only be used to immediately parse information or some user-defined object out of it. If an independent deep copy is needed you can call `Bytes::ToString()`.

Preconditions:

* `HasValue() == true`

<span class='declaration' id='iterator-deletevalue'>`void DeleteValue()`</span>

Marks the value the iterator currently points to as deleted.

Preconditions:

* `HasValue() == true`

Postconditions:

* `HasValue() == false`

<span class='declaration' id='iterator-next'>`void Next()`</span>

Moves the iterator to the next value, if any.

## class Map

```cpp
#include <multimap/Map.hpp>
namespace multimap
```

This class implements a 1:n key-value store where each key is associated with a list of values.

Members        |
--------------:|----------------------------------------------------------------
`typedef`      | [`Iterator<false> ListIter`](#map-listiter)
`typedef`      | [`Iterator<true> ConstListIter`](#map-constlistiter)
               | [`Map()`](#map-map)
               | [`~Map()`](#map-~map)
               | [`Map(const boost::filesystem::path& directory, const Options& options)`](#map-map2)
`void`         | [`Open(const boost::filesystem::path& directory, const Options& options)`](#map-open)
`void`         | [`Put(const Bytes& key, const Bytes& value)`](#map-put)
`ConstListIter`| [`Get(const Bytes& key) const`](#map-get)
`ListIter`     | [`GetMutable(const Bytes& key)`](#map-getmutable)
`bool`         | [`Contains(const Bytes& key) const`](#map-contains)
`std::size_t`  | [`Delete(const Bytes& key)`](#map-delete)
`std::size_t`  | [`DeleteAll(const Bytes& key, Callables::Predicate predicate)`](#map-deleteall)
`std::size_t`  | [`DeleteAllEqual(const Bytes& key, const Bytes& value)`](#map-deleteallequal)
`bool`         | [`DeleteFirst(const Bytes& key, Callables::Predicate predicate)`](#map-deletefirst)
`bool`         | [`DeleteFirstEqual(const Bytes& key, const Bytes& value)`](#map-deletefirstequal)
`std::size_t`  | [`ReplaceAll(const Bytes& key, Callables::Function function)`](#map-replaceall)
`std::size_t`  | [`ReplaceAllEqual(const Bytes& key, const Bytes& old_value, const Bytes& new_value)`](#map-replaceallequal)
`bool`         | [`ReplaceFirst(const Bytes& key, Callables::Function function)`](#map-replacefirst)
`bool`         | [`ReplaceFirstEqual(const Bytes& key, const Bytes& old_value, const Bytes& new_value)`](#map-replacefirstequal)
`void`         | [`ForEachKey(Callables::Procedure procedure) const`](#map-foreachkey)
`void`         | [`ForEachValue(const Bytes& key, Callables::Procedure procedure) const`](#map-foreachvalue-procedure)
`void`         | [`ForEachValue(const Bytes& key, Callables::Predicate predicate) const`](#map-foreachvalue-predicate)
`std::map<std::string, std::string>` | [`GetProperties() const`](#map-getproperties)
`std::size_t`  | [`max_key_size() const`](#map-max-key-size)
`std::size_t`  | [`max_value_size() const`](#map-max-value-size)

Functions      |
--------------:|----------------------------------------------------------------
`void`         | [`Optimize(const boost::filesystem::path& from, const boost::filesystem::path& to)`](#map-optimize)
`void`         | [`Optimize(const boost::filesystem::path& from, const boost::filesystem::path& to, std::size_t new_block_size)`](#map-optimize2)
`void`         | [`Optimize(const boost::filesystem::path& from, const boost::filesystem::path& to, Callables::Compare compare)`](#map-optimize3)
`void`         | [`Optimize(const boost::filesystem::path& from, const boost::filesystem::path& to, Callables::Compare compare, std::size_t new_block_size)`](#map-optimize3)
`void`         | [`Import(const boost::filesystem::path& directory, const boost::filesystem::path& file)`](#map-import)
`void`         | [`Import(const boost::filesystem::path& directory, const boost::filesystem::path& file, bool create_if_missing)`](#map-import2)
`void`         | [`Import(const boost::filesystem::path& directory, const boost::filesystem::path& file, bool create_if_missing, std::size_t block_size)`](#map-import3)
`void`         | [`Export(const boost::filesystem::path& directory, const boost::filesystem::path& file)`](#map-export)

<span class='declaration' id='map-listiter'>
 `typedef`&nbsp;&nbsp;[`Iterator<false>`](#class-iterator)&nbsp;&nbsp;`ListIter`
</span>

An iterator type to iterate a mutable list. All operations declared in the class template [`Iterator<bool>`](#class-iterator) that can mutate the underlying list are enabled.

<span class='declaration' id='map-constlistiter'>
 `typedef`&nbsp;&nbsp;[`Iterator<true>`](#class-iterator)&nbsp;&nbsp;`ConstListIter`
</span>

An iterator type to iterate a immutable list. All operations declared in the class template [`Iterator<bool>`](#class-iterator) that can mutate the underlying list are disabled.

<span class='declaration' id='map-map'>`Map()`</span>

Creates a default instance which is not associated with a physical map.

<span class='declaration' id='map-~map'>`~Map()`</span>

If associated with a physical map the destructor flushes all data to disk and ensures that the map is stored in consistent state.

Preconditions:

* No list is in locked state, i.e. there is no iterator object pointing to an existing list.

<span class='declaration' id='map-map2'>
 `Map(const boost::filesystem::path& directory,`
 <script>nbsp(10)</script>`const`&nbsp;&nbsp;[`Options`](#class-options)`& options)`
</span>

Creates a new instance and opens the map located in `directory`. If the map does not exist and `options.create_if_missing` is set to `true` a new map will be created.

Throws `std::exception` if:

* `directory` does not exist.
* `directory` does not contain a map and `options.create_if_missing` is `false`.
* `directory` contains a map and `options.error_if_exists` is `true`.
* `options.block_size` is not a power of two.

<span class='declaration' id='map-open'>
 `void Open(`
 <script>nbsp(20)</script>`const boost::filesystem::path& directory,`
 <script>nbsp(20)</script>`const`&nbsp;&nbsp;[`Options`](#class-options)`& options)`
</span>

Opens the map located in `directory`. If the map does not exist and `options.create_if_missing` is set to `true` a new map will be created.

Throws `std::exception` if:

* `directory` does not exist.
* `directory` does not contain a map and `options.create_if_missing` is `false`.
* `directory` contains a map and `options.error_if_exists` is `true`.
* `options.block_size` is not a power of two.

<span class='declaration' id='map-put'>
 `void Put(const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& key, const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& value)`
</span>

Appends `value` to the end of the list associated with `key`.

Throws `std::exception` if:

* `key.size() > max_key_size()`
* `value.size() > max_value_size()`

<span class='declaration' id='map-get'>
 [`ConstListIter`](#map-constlistiter)&nbsp;&nbsp;`Get(const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& key) const`
</span>

Returns a read-only iterator to the list associated with `key`. If no such mapping exists the list is considered to be empty. If the list is not empty a reader lock will be acquired to synchronize concurrent access to it. Thus, multiple threads can read the list at the same time. Once acquired, the lock is automatically released when the lifetime of the iterator ends and its destructor is called. If the list is currently locked exclusively by a writer lock, see `GetMutable()`, the method will block until the lock is released.

<span class='declaration' id='map-getmutable'>
 [`ListIter`](#map-listiter)&nbsp;&nbsp;`GetMutable(const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& key)`
</span>

Returns a read-write iterator to the list associated with `key`. If no such mapping exists the list is considered to be empty. If the list is not empty a writer lock will be acquired to synchronize concurrent access to it. Only one thread can acquire a writer lock at a time, since it requires exclusive access. Once acquired, the lock is automatically released when the lifetime of the iterator ends and its destructor is called. If the list is currently locked, either by a reader or writer lock, the method will block until the lock is released.

<span class='declaration' id='map-contains'>
 `bool Contains(const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& key)`
</span>

Returns `true` if the list associated with `key` contains at least one value, returns `false` otherwise. If the key does not exist the list is considered to be empty. If a non-empty list is currently locked, the method will block until the lock is released.

<span class='declaration' id='map-delete'>
 `std::size_t Delete(const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& key)`
</span>

Deletes all values for `key` by clearing the associated list. This method will block until a writer lock can be acquired.

Returns: the number of deleted values.

<span class='declaration' id='map-deleteall'>
 `std::size_t DeleteAll(`
 <script>nbsp(20)</script>`const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& key,`
 <script>nbsp(20)</script>[`Callables::Predicate`](#callables-predicate)&nbsp;&nbsp;`predicate)`
</span>

Deletes all values in the list associated with `key` for which `predicate` yields `true`. This method will block until a writer lock can be acquired.

Returns: the number of deleted values.

<span class='declaration' id='map-deleteallequal'>
 `std::size_t DeleteAllEqual(`
 <script>nbsp(20)</script>`const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& key,`
 <script>nbsp(20)</script>`const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& value)`
</span>

Deletes all values in the list associated with `key` which are equal to `value` according to `operator==(const Bytes&, const Bytes&)`. This method will block until a writer lock can be acquired.

Returns: the number of deleted values.

<span class='declaration' id='map-deletefirst'>
 `bool DeleteFirst(`
 <script>nbsp(20)</script>`const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& key,`
 <script>nbsp(20)</script>[`Callables::Predicate`](#callables-predicate)&nbsp;&nbsp;`predicate)`
</span>

Deletes the first value in the list associated with `key` for which `predicate` yields `true`. This method will block until a writer lock can be acquired.

Returns: `true` if a value was deleted, `false` otherwise.

<span class='declaration' id='map-deletefirstequal'>
 `bool DeleteFirstEqual(`
 <script>nbsp(20)</script>`const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& key,`
 <script>nbsp(20)</script>`const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& value)`
</span>

Deletes the first value in the list associated with `key` which is equal to `value` according to `operator==(const Bytes&, const Bytes&)`. This method will block until a writer lock can be acquired.

Returns: `true` if a value was deleted, `false` otherwise.

<span class='declaration' id='map-replaceall'>
 `std::size_t ReplaceAll(`
 <script>nbsp(20)</script>`const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& key,`
 <script>nbsp(20)</script>[`Callables::Function`](#callables-function)&nbsp;&nbsp;`function)`
</span>

Replaces all values in the list associated with `key` by the result of invoking `function`. If the result of `function` is an empty string no replacement is performed. A replacement does not happen in-place. Instead, the old value is marked as deleted and the new value is appended to the end of the list. Future releases will support in-place replacements. This method will block until a writer lock can be acquired.

Returns: the number of replaced values.

<span class='declaration' id='map-replaceallequal'>
 `std::size_t ReplaceAllEqual(`
 <script>nbsp(20)</script>`const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& key,`
 <script>nbsp(20)</script>`const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& old_value,`
 <script>nbsp(20)</script>`const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& new_value)`
</span>

Replaces all values in the list associated with `key` which are equal to `old_value` according to `operator==(const Bytes&, const Bytes&)` by `new_value`. A replacement does not happen in-place. Instead, the old value is marked as deleted and the new value is appended to the end of the list. Future releases will support in-place replacements. This method will block until a writer lock can be acquired.

Returns: the number of replaced values.

<span class='declaration' id='map-replacefirst'>
 `bool ReplaceFirst(`
 <script>nbsp(20)</script>`const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& key,`
 <script>nbsp(20)</script>[`Callables::Function`](#callables-function)&nbsp;&nbsp;`function)`
</span>

Replaces the first value in the list associated with `key` by the result of invoking `function`. If the result of `function` is an empty string no replacement is performed. The replacement does not happen in-place. Instead, the old value is marked as deleted and the new value is appended to the end of the list. Future releases will support in-place replacements. This method will block until a writer lock can be acquired.

Returns: `true` if a value was replaced, `false` otherwise.

<span class='declaration' id='map-replacefirstequal'>
 `bool ReplaceFirstEqual(`
 <script>nbsp(20)</script>`const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& key,`
 <script>nbsp(20)</script>`const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& old_value,`
 <script>nbsp(20)</script>`const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& new_value)`
</span>

Replaces the first value in the list associated with `key` which is equal to `old_value` according to `operator==(const Bytes&, const Bytes&)` by `new_value`. The replacement does not happen in-place. Instead, the old value is marked as deleted and the new value is appended to the end of the list. Future releases will support in-place replacements. This method will block until a writer lock can be acquired.

Returns: `true` if a value was replaced, `false` otherwise.

<span class='declaration' id='map-foreachkey'>
 `void ForEachKey(`
 <script>nbsp(20)</script>[`Callables::Procedure`](#callables-procedure)&nbsp;&nbsp;`procedure) const`
</span>

Applies `procedure` to each key of the map whose associated list is not empty. For the time of execution the entire map is locked for read-only operations. It is possible to keep a reference to the map within `procedure` and to call other read-only operations such as `Get()`. However, trying to call mutable operations such as `GetMutable()` will cause a deadlock.

<span class='declaration' id='map-foreachvalue-procedure'>
 `void ForEachValue(`
 <script>nbsp(20)</script>`const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& key,`
 <script>nbsp(20)</script>[`Callables::Procedure`](#callables-procedure)&nbsp;&nbsp;`procedure) const`
</span>

Applies `procedure` to each value in the list associated with `key`. This is a shorthand for requesting a read-only iterator via `Get(key)` followed by an application of `procedure` to each value obtained via `ConstListIter::GetValue()`. This method will block until a reader lock for the list in question can be acquired.

<span class='declaration' id='map-foreachvalue-predicate'>
 `void ForEachValue(`
 <script>nbsp(20)</script>`const`&nbsp;&nbsp;[`Bytes`](#class-bytes)`& key,`
 <script>nbsp(20)</script>[`Callables::Predicate`](#callables-predicate)&nbsp;&nbsp;`predicate) const`
</span>

Applies `predicate` to each value in the list associated with `key` until `predicate` yields `false`. This is a shorthand for requesting a read-only iterator via `Get(key)` followed by an application of `predicate` to each value obtained via `ConstListIter::GetValue()` until `predicate` yields `false`. This method will block until a reader lock for the list in question can be acquired.

<span class='declaration' id='map-getproperties'>
 `std::map<std::string, std::string> GetProperties() const`
</span>

Returns a list of properties which describe the state of the map similar to those written to the `multimap.properties` file. This method will only look at lists which are currently not locked to be non-blocking. Therefore, the returned values will be an approximation. For the time of execution the map is locked for read-only operations.

<span class='declaration' id='map-max-key-size'>`std::size_t max_key_size() const`</span>

Returns the maximum size of a key in bytes which may be put. Currently this is `65536` bytes.

<span class='declaration' id='map-max-value-size'>`std::size_t max_value_size() const`</span>

Returns the maximum size of a value in bytes which may be put. Currently this is `options.block_size - 2` bytes.

<span class='declaration' id='map-optimize'>
 `void Optimize(`
 <script>nbsp(20)</script>`const boost::filesystem::path& from,`
 <script>nbsp(20)</script>`const boost::filesystem::path& to)`
</span>

Copies the map located in the directory denoted by `from` to the directory denoted by `to` performing the following optimizations:

* Defragmentation. All blocks which belong to the same list are written sequentially to disk which improves locality and leads to better read performance.
* Garbage collection. Values that are marked as deleted won't be copied which reduces the size of the new map and also improves locality.

Throws `std::exception` if:

* `from` or `to` are not directories.
* `from` does not contain a map.
* the map in `from` is locked.
* `to` already contains a map.
* `to` is not writable.

<span class='declaration' id='map-optimize2'>
 `void Optimize(`
 <script>nbsp(20)</script>`const boost::filesystem::path& from,`
 <script>nbsp(20)</script>`const boost::filesystem::path& to`
 <script>nbsp(20)</script>`std::size_t new_block_size)`
</span>

Same as [`Optimize(from, to)`](#map-optimize) but sets the block size of the new map to `new_block_size`.

Throws `std::exception` if:

* see [`Optimize(from, to)`](#map-optimize)
* `new_block_size` is not a power of two.

<span class='declaration' id='map-optimize3'>
 `void Optimize(`
 <script>nbsp(20)</script>`const boost::filesystem::path& from,`
 <script>nbsp(20)</script>`const boost::filesystem::path& to,`
 <script>nbsp(20)</script>[`Callables::Compare`](#callables-compare)&nbsp;&nbsp;`compare)`
</span>

Same as [`Optimize(from, to)`](#map-optimize) but sorts each list before writing using `compare` as the sorting criterion.

Throws `std::exception` if:

* see [`Optimize(from, to)`](#map-optimize)

<span class='declaration' id='map-optimize4'>
 `void Optimize(`
 <script>nbsp(20)</script>`const boost::filesystem::path& from,`
 <script>nbsp(20)</script>`const boost::filesystem::path& to,`
 <script>nbsp(20)</script>[`Callables::Compare`](#callables-compare)&nbsp;&nbsp;`compare,`
 <script>nbsp(20)</script>`std::size_t new_block_size)`
</span>

Same as [`Optimize(from, to, compare)`](#map-optimize3) but sets the block size of the new map to `new_block_size`.

Throws `std::exception` if:

* see [`Optimize(from, to, compare)`](#map-optimize3)
* `new_block_size` is not a power of two.

<span class='declaration' id='map-import'>
 `void Import(`
 <script>nbsp(20)</script>`const boost::filesystem::path& directory,`
 <script>nbsp(20)</script>`const boost::filesystem::path& file)`
</span>

Imports key-value pairs from a Base64-encoded text file denoted by `file` into the map located in the directory denoted by `directory`.

Preconditions:

* The content in `file` follows the format described in [Import / Export](overview.md#import-export).

Throws `std::exception` if:

* `directory` does not exist.
* `directory` does not contain a map.
* the map in `directory` is locked.
* `file` is not a regular file.

<span class='declaration' id='map-import2'>
 `void Import(`
 <script>nbsp(20)</script>`const boost::filesystem::path& directory,`
 <script>nbsp(20)</script>`const boost::filesystem::path& file`
 <script>nbsp(20)</script>`bool create_if_missing)`
</span>

Same as [`Import(directory, file)`](#map-import) but creates a new map with default block size if the directory denoted by `directory` does not contain a map and `create_if_missing` is `true`.

Preconditions:

* see [`Import(directory, file)`](#map-import)

Throws `std::exception` if:

* see [`Import(directory, file)`](#map-import)

<span class='declaration' id='map-import3'>
 `void Import(`
 <script>nbsp(20)</script>`const boost::filesystem::path& directory,`
 <script>nbsp(20)</script>`const boost::filesystem::path& file,`
 <script>nbsp(20)</script>`bool create_if_missing,`
 <script>nbsp(20)</script>`std::size_t block_size)`
</span>

Same as [`Import(directory, file, create_if_missing)`](#map-import2) but sets the block size of a newly created map to `block_size`.

Preconditions:

* see [`Import(directory, file, create_if_missing)`](#map-import2)

Throws `std::exception` if:

* see [`Import(directory, file, create_if_missing)`](#map-import2)
* `block_size` is not a power of two.

<span class='declaration' id='map-export'>
 `void Export(`
 <script>nbsp(20)</script>`const boost::filesystem::path& directory,`
 <script>nbsp(20)</script>`const boost::filesystem::path& file)`
</span>

Exports all key-value pairs from the map located in the directory denoted by `directory` to a Base64-encoded text file denoted by `file`. If the file already exists, its content will be overridden.

Postconditions:

* The content in `file` follows the format described in [Import / Export](overview.md#import-export).

Throws `std::exception` if:

* `directory` does not exist.
* `directory` does not contain a map.
* the map in `directory` is locked.
* `file` cannot be created.

## class Options

```cpp
#include <multimap/Options.hpp>
namespace multimap
```

This class is a pure data holder used to configure an instantiation of class [`Map`](#class-map).

Members       |
-------------:|-----------------------------------------------------------------
`std::size_t` | [`block_size`](#options-block-size)
`bool`        | [`create_if_missing`](#options-create-if-missing)
`bool`        | [`error_if_exists`](#options-error-if-exists)
`bool`        | [`write_only_mode`](#options-write-only-mode)

<span class='declaration' id='options-block-size'>`std::size_t block_size`</span>

Determines the block size of a newly created map. The value is ignored if the map already exists when opened. The value must be a power of two. Have a look at [Choosing the block size](overview.md#choosing-the-block-size) for more information.

Default: `512`

<span class='declaration' id='options-create_if_missing'>`bool create_if_missing`</span>

Determines whether a map has to be created if it does not exist.

Default: `false`

<span class='declaration' id='options-error_if_exists'>`bool error_if_exists`</span>

Determines whether an already existing map should be treated as an error.

Default: `false`

<span class='declaration' id='options-write_only_mode'>`bool write_only_mode`</span>

Determines if the map should be opened in write-only mode. This will enable some optimizations for putting a large number of values, but will disable the ability to retrieve values. Users normally should leave this parameter alone.

Default: `false`
