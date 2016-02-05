/*
 * This file is part of Multimap.  http://multimap.io
 *
 * Copyright (C) 2015-2016  Martin Trenkmann
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

package io.multimap;

import io.multimap.Callables.Function;
import io.multimap.Callables.Predicate;
import io.multimap.Callables.Procedure;

import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.nio.file.Path;
import java.nio.file.Paths;

/**
 * This class implements a 1:n key-value store where each key is associated with a list of values.
 * For more information please visit the
 * <a href="http://multimap.io/cppreference/#maphpp">C++ Reference for class Map</a>.
 * 
 * @author Martin Trenkmann
 * @since 0.3.0
 */
public class Map implements AutoCloseable {

  static {
    final String LIBNAME = "multimap-jni";
    try {
      System.loadLibrary(LIBNAME);
    } catch (Exception e) {
      System.err.println(e);
    }
  }
  
  private static final Charset UTF8 = Charset.forName("UTF-8");

  /**
   * This type provides static methods for obtaining system limitations. Those limits which define
   * constraints on user supplied data also serve as preconditions.
   * 
   * @author Martin Trenkmann
   * @since 0.3.0
   */
  static class Limits {

    /**
     * Returns the maximum size in number of bytes for a key to put.
     */
    static int maxKeySize() {
      return Native.maxKeySize();
    }

    /**
     * Returns the maximum size in number of bytes for a value to put.
     */
    static int maxValueSize() {
      return Native.maxValueSize();
    }

    static class Native {
      static native int maxKeySize();
      static native int maxValueSize();
    }
  }

  private ByteBuffer self;

  private Map(ByteBuffer nativePtr) {
    Check.notNull(nativePtr);
    Check.notZero(nativePtr.capacity());
    self = nativePtr;
  }

  /**
   * Opens an already existing map located in {@code directory}.
   * 
   * <p><b>Acquires</b> a directory lock on directory.</p>
   * 
   * @throws Exception if one of the following is true:
   * <ul>
   * <li>the directory does not exist</li>
   * <li>the directory cannot be locked</li>
   * <li>the directory does not contain a map</li>
   * </ul>
   */
  public Map(String directory) throws Exception {
    this(Paths.get(directory), new Options());
  }
  
  /**
   * @see Map#Map(String)
   */
  public Map(Path directory) throws Exception {
    this(directory, new Options());
  }
  
  /**
   * Opens or creates a map in {@code directory}. For the latter, you need to set
   * {@code options.createIfMissing(true)}. If an error should be raised in case the map already
   * exists, set {@code options.setErrorIfExists(true)}. When a new map is created other fields in
   * options are used to configure the map's block size and number of partitions. See
   * {@link Options} for more information.
   * 
   * <p><b>Acquires</b> a directory lock on directory.</p>
   * 
   * @throws Exception if one of the following is true:
   * <ul>
   * <li>the directory does not exist</li>
   * <li>the directory cannot be locked</li>
   * </ul>
   * when {@code options.isCreateIfMissing()} is {@code false} (which is the default)
   * <ul>
   * <li>the directory does not contain a map</li>
   * </ul>
   * when {@code options.isCreateIfMissing()} return {@code true} and no map exists
   * <ul>
   * <li>{@code options.getBlockSize()} is zero</li>
   * <li>{@code options.getBlockSize()} is not a power of two</li>
   * </ul>
   * when {@code options.isErrorIfExists()} is {@code true}
   * <ul>
   * <li>the directory already contains a map</li>
   * </ul>
   */
  public Map(String directory, Options options) throws Exception {
    this(Paths.get(directory), options);
  }
  
  /**
   * @see Map#Map(String, Options)
   */
  public Map(Path directory, Options options) throws Exception {
    this(Native.newMap(directory.toString(), options));
  }

  /**
   * Appends {@code value} to the end of the list associated with {@code key}.
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a writer lock on the map object.</li>
   * <li>a writer lock on the list associated with {@code key}.</li>
   * </ul>
   * 
   * @throws Exception if one of the following is true:
   * <ul>
   * <li>{@code key.size() > Map.Limits.maxKeySize()}</li>
   * <li>{@code value.size() > Map.Limits.maxValueSize()}</li>
   * <li>the map was opened in read-only mode</li>
   * </ul>
   */
  public void put(byte[] key, byte[] value) throws Exception {
    Check.notNull(key);
    Check.notNull(value);
    Native.put(self, key, value);
  }
  
  /**
   * Same as {@link #put(byte[], byte[])}, but takes the key as {@link String} instead of
   * {@code byte[]}. Internally the key is converted to a byte array by calling
   * {@link String#getBytes(Charset)} using UTF-8 as the character set.
   * 
   * @since 0.4.0
   */
  public void put(String key, byte[] value) throws Exception {
    Native.put(self, key.getBytes(UTF8), value);
  }
  
  /**
   * Same as {@link #put(byte[], byte[])}, but takes both the key and the value as {@link String}
   * instead of {@code byte[]}. Internally the key and the value are converted to byte arrays by
   * calling {@link String#getBytes(Charset)} using UTF-8 as the character set.
   * 
   * @since 0.4.0
   */
  public void put(String key, String value) throws Exception {
    Native.put(self, key.getBytes(UTF8), value.getBytes(UTF8));
  }

  /**
   * Returns a read-only iterator for the list associated with {@code key}. If the key does not
   * exist, an empty iterator that has no values is returned. A non-empty iterator owns a reader
   * lock on the underlying list and therefore must be closed via {@link Iterator#close()} when it
   * is not needed anymore. Not closing iterators leads to deadlocks sooner or later.
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a reader lock on the list associated with {@code key}.</li>
   * </ul>
   */
  @SuppressWarnings("resource")
  public Iterator get(byte[] key) {
    Check.notNull(key);
    ByteBuffer nativePtr = Native.get(self, key);
    return (nativePtr != null) ? new Iterator(nativePtr) : Iterator.EMPTY;
  }
  
  /**
   * Same as {@link #get(byte[])}, but takes the key as {@link String} instead of {@code byte[]}.
   * Internally the key is converted to a byte array by calling {@link String#getBytes(Charset)}
   * using UTF-8 as the character set.
   * 
   * @since 0.4.0
   */
  public Iterator get(String key) {
    return get(key.getBytes(UTF8));
  }
  
  /**
   * Returns {@code true} if {@code key} is associated with at least one value, {@code false}
   * otherwise.
   */
  public boolean containsKey(byte[] key) {
    Check.notNull(key);
    return Native.containsKey(self, key);
  }
  
  /**
   * Same as {@link #containsKey(byte[])}, but takes the key as {@link String} instead of
   * {@code byte[]}. Internally the key is converted to a byte array by calling
   * {@link String#getBytes(Charset)} using UTF-8 as the character set.
   * 
   * @since 0.4.0
   */
  public boolean containsKey(String key) {
    return containsKey(key.getBytes(UTF8));
  }

  /**
   * Removes all values associated with {@code key}.
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on the list associated with {@code key}.</li>
   * </ul>
   * 
   * @return {@code true} if any values have been removed, {@code false} otherwise.
   */
  public boolean removeKey(byte[] key) {
    Check.notNull(key);
    return Native.removeKey(self, key);
  }
  
  /**
   * Same as {@link #removeKey(byte[])}, but takes the key as {@link String} instead of
   * {@code byte[]}. Internally the key is converted to a byte array by calling
   * {@link String#getBytes(Charset)} using UTF-8 as the character set.
   * 
   * @since 0.4.0
   */
  public boolean removeKey(String key) {
    return removeKey(key.getBytes(UTF8));
  }

  /**
   * Removes all values associated with keys for which {@code predicate} yields {@code true}. The
   * predicate can be any callable that implements the {@link Predicate} interface.
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on lists associated with matching keys.</li>
   * </ul>
   * 
   * @return the number of keys for which any values have been removed.
   */
  public long removeKeys(Predicate predicate) {
    return Native.removeKeys(self, predicate);
  }

  /**
   * Removes the first value from the list associated with {@code key} that is equal to
   * {@code value} after to byte-wise comparison.
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on the list associated with {@code key}.</li>
   * </ul>
   * 
   * @return {@code true} if any value has been removed, {@code false} otherwise.
   */
  public boolean removeValue(byte[] key, byte[] value) {
    Check.notNull(key);
    Check.notNull(value);
    return Native.removeValue(self, key, value);
  }
  
  /**
   * Same as {@link #removeValue(byte[], byte[])}, but takes the key as {@link String} instead of
   * {@code byte[]}. Internally the key is converted to a byte array by calling
   * {@link String#getBytes(Charset)} using UTF-8 as the character set.
   * 
   * @since 0.4.0
   */
  public boolean removeValue(String key, byte[] value) {
    return removeValue(key.getBytes(UTF8), value);
  }
  
  /**
   * Same as {@link #removeValue(byte[], byte[])}, but takes both the key and the value as
   * {@link String} instead of {@code byte[]}. Internally the key and the value are converted to
   * byte arrays by calling {@link String#getBytes(Charset)} using UTF-8 as the character set.
   * 
   * @since 0.4.0
   */
  public boolean removeValue(String key, String value) {
    return removeValue(key.getBytes(UTF8), value.getBytes(UTF8));
  }

  /**
   * Removes the first value from the list associated with {@code key} for which {@code predicate}
   * yields {@code true}. The predicate can be any callable that implements the {@link Predicate}
   * interface.
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on the list associated with {@code key}.</li>
   * </ul>
   * 
   * @return {@code true} if any value has been removed, {@code false} otherwise.
   */
  public boolean removeValue(byte[] key, Predicate predicate) {
    Check.notNull(key);
    Check.notNull(predicate);
    return Native.removeValue(self, key, predicate);
  }
  
  /**
   * Same as {@link #removeValue(byte[], Callables.Predicate)}, but takes the key as {@link String}
   * instead of {@code byte[]}. Internally the key is converted to a byte array by calling
   * {@link String#getBytes(Charset)} using UTF-8 as the character set.
   * 
   * @since 0.4.0
   */
  public boolean removeValue(String key, Predicate predicate) {
    return removeValue(key.getBytes(UTF8), predicate);
  }


  /**
   * Removes all values from the list associated with {@code key} that are equal to {@code value}
   * after to byte-wise comparison.
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on the list associated with {@code key}.</li>
   * </ul>
   * 
   * @return the number of values removed.
   */
  public long removeValues(byte[] key, byte[] value) {
    Check.notNull(key);
    Check.notNull(value);
    return Native.removeValues(self, key, value);
  }
  
  /**
   * Same as {@link #removeValues(byte[], byte[])}, but takes the key as {@link String} instead of
   * {@code byte[]}. Internally the key is converted to a byte array by calling
   * {@link String#getBytes(Charset)} using UTF-8 as the character set.
   * 
   * @since 0.4.0
   */
  public long removeValues(String key, byte[] value) {
    return removeValues(key.getBytes(UTF8), value);
  }
  
  /**
   * Same as {@link #removeValues(byte[], byte[])}, but takes both the key and the value as
   * {@link String} instead of {@code byte[]}. Internally the key and the value are converted to
   * byte arrays by calling {@link String#getBytes(Charset)} using UTF-8 as the character set.
   * 
   * @since 0.4.0
   */
  public long removeValues(String key, String value) {
    return removeValues(key.getBytes(UTF8), value.getBytes(UTF8));
  }

  /**
   * Removes all values from the list associated with {@code key} for which {@code predicate}
   * yields {@code true}. The predicate can be any callable that implements the {@link Predicate}
   * interface.
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on the list associated with {@code key}.</li>
   * </ul>
   * 
   * @return the number of values removed.
   */
  public long removeValues(byte[] key, Predicate predicate) {
    Check.notNull(key);
    Check.notNull(predicate);
    return Native.removeValues(self, key, predicate);
  }
  
  /**
   * Same as {@link #removeValues(byte[], Callables.Predicate)}, but takes the key as {@link String}
   * instead of {@code byte[]}. Internally the key is converted to a byte array by calling
   * {@link String#getBytes(Charset)} using UTF-8 as the character set.
   * 
   * @since 0.4.0
   */
  public long removeValues(String key, Predicate predicate) {
    return removeValues(key.getBytes(UTF8), predicate);
  }

  /**
   * Replaces the first value in the list associated with {@code key} that is equal to
   * {@code oldValue} by {@code newValue}.
   *
   * <p>Note that a replace operation is actually implemented in terms of a remove of the old value
   * followed by an insert/put of the new value. Thus, the new value is always the last value in
   * the list. In other words, the replacement is not in-place.</p>
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on the list associated with {@code key}.</li>
   * </ul>
   * 
   * @return {@code true} if any value has been replaced, {@code false} otherwise.
   */
  public boolean replaceValue(byte[] key, byte[] oldValue, byte[] newValue) {
    Check.notNull(key);
    Check.notNull(oldValue);
    Check.notNull(newValue);
    return Native.replaceValue(self, key, oldValue, newValue);
  }
  
  /**
   * Same as {@link #replaceValue(byte[], byte[], byte[])}, but takes the key as {@link String}
   * instead of {@code byte[]}. Internally the key is converted to a byte array by calling
   * {@link String#getBytes(Charset)} using UTF-8 as the character set.
   * 
   * @since 0.4.0
   */
  public boolean replaceValue(String key, byte[] oldValue, byte[] newValue) {
    return replaceValue(key.getBytes(UTF8), oldValue, newValue);
  }
  
  /**
   * Same as {@link #replaceValue(byte[], byte[], byte[])}, but takes all arguments as
   * {@link String} instead of {@code byte[]}. Internally the arguments are converted to byte arrays
   * by calling {@link String#getBytes(Charset)} using UTF-8 as the character set.
   * 
   * @since 0.4.0
   */
  public boolean replaceValue(String key, String oldValue, String newValue) {
    return replaceValue(key.getBytes(UTF8), oldValue.getBytes(UTF8), newValue.getBytes(UTF8));
  }

  /**
   * Replaces the first value in the list associated with {@code key} by the result of invoking
   * {@code map}. Values for which {@code map} returns {@code null} are not replaced. The map
   * function can be any callable that implements the {@link Function} interface.
   * 
   * <p>Note that a replace operation is actually implemented in terms of a remove of the old value
   * followed by an insert/put of the new value. Thus, the new value is always the last value in
   * the list. In other words, the replacement is not in-place.</p>
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on the list associated with {@code key}.</li>
   * </ul>
   * 
   * @return {@code true} if any value has been replaced, {@code false} otherwise.
   */
  public boolean replaceValue(byte[] key, Function map) {
    Check.notNull(key);
    Check.notNull(map);
    return Native.replaceValue(self, key, map);
  }
  
  /**
   * Same as {@link #replaceValue(byte[], Callables.Function)}, but takes the key as {@link String}
   * instead of {@code byte[]}. Internally the key is converted to a byte array by calling
   * {@link String#getBytes(Charset)} using UTF-8 as the character set.
   * 
   * @since 0.4.0
   */
  public boolean replaceValue(String key, Function map) {
    return replaceValue(key.getBytes(UTF8), map);
  }

  /**
   * Replaces each value in the list associated with {@code key} that is equal to {@code oldValue}
   * by {@code newValue}.
   * 
   * <p>Note that a replace operation is actually implemented in terms of a remove of the old value
   * followed by an insert/put of the new value. Thus, the new value is always the last value in
   * the list. In other words, the replacement is not in-place.</p>
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on the list associated with {@code key}.</li>
   * </ul>
   * 
   * @return the number of values replaced.
   */
  public long replaceValues(byte[] key, byte[] oldValue, byte[] newValue) {
    Check.notNull(key);
    Check.notNull(oldValue);
    Check.notNull(newValue);
    return Native.replaceValues(self, key, oldValue, newValue);
  }
  
  /**
   * Same as {@link #replaceValues(byte[], byte[], byte[])}, but takes the key as {@link String}
   * instead of {@code byte[]}. Internally the key is converted to a byte array by calling
   * {@link String#getBytes(Charset)} using UTF-8 as the character set.
   * 
   * @since 0.4.0
   */
  public long replaceValues(String key, byte[] oldValue, byte[] newValue) {
    return replaceValues(key.getBytes(UTF8), oldValue, newValue);
  }
  
  /**
   * Same as {@link #replaceValues(byte[], byte[], byte[])}, but takes all arguments as
   * {@link String} instead of {@code byte[]}. Internally the arguments are converted to byte arrays
   * by calling {@link String#getBytes(Charset)} using UTF-8 as the character set.
   * 
   * @since 0.4.0
   */
  public long replaceValues(String key, String oldValue, String newValue) {
    return replaceValues(key.getBytes(UTF8), oldValue.getBytes(UTF8), newValue.getBytes(UTF8));
  }
  
  /**
   * Replaces each value in the list associated with {@code key} by the result of invoking
   * {@code map}. Values for which {@code map} returns {@code null} are not replaced. The map
   * function can be any callable that implements the {@link Function} interface.
   * 
   * <p>Note that a replace operation is actually implemented in terms of a remove of the old value
   * followed by an insert/put of the new value. Thus, the new value is always the last value in
   * the list. In other words, the replacement is not in-place.</p>
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on the list associated with {@code key}.</li>
   * </ul>
   * 
   * @return the number of values replaced.
   */
  public long replaceValues(byte[] key, Function map) {
    Check.notNull(key);
    Check.notNull(map);
    return Native.replaceValues(self, key, map);
  }
  
  /**
   * Same as {@link #replaceValues(byte[], Callables.Function)}, but takes the key as {@link String}
   * instead of {@code byte[]}. Internally the key is converted to a byte array by calling
   * {@link String#getBytes(Charset)} using UTF-8 as the character set.
   * 
   * @since 0.4.0
   */
  public long replaceValues(String key, Function map) {
    return replaceValues(key.getBytes(UTF8), map);
  }

  /**
   * Applies {@code process} to each key whose list is not empty. The process argument can be any
   * callable that implements the {@link Procedure} interface. 
   * 
   * <p><b>Acquires</b> a reader lock on the map object.</p>
   */
  public void forEachKey(Procedure process) {
    Check.notNull(process);
    Native.forEachKey(self, process);
  }

  /**
   * Applies {@code process} to each value associated with {@code key}. The process argument can be
   * any callable that implements the {@link Procedure} interface. 
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on the list associated with {@code key}.</li>
   * </ul>
   */
  public void forEachValue(byte[] key, Procedure process) {
    Check.notNull(process);
    Native.forEachValue(self, key, process);
  }
  
  /**
   * Same as {@link #forEachValue(byte[], Callables.Procedure)}, but takes the key as {@link String}
   * instead of {@code byte[]}. Internally the key is converted to a byte array by calling
   * {@link String#getBytes(Charset)} using UTF-8 as the character set.
   * 
   * @since 0.4.0
   */
  public void forEachValue(String key, Procedure process) {
    forEachValue(key.getBytes(UTF8), process);
  }

  /*
   * The method `forEachEntry` as provided in the C++ version of this library is currently not
   * supported. Consider submitting a feature request if you really need it.
   */

  /*
   * The method `getStats` as provided in the C++ version of this library is currently not
   * supported. Consider submitting a feature request if you really need it.
   */

  /*
   * The method `getTotalStats` as provided in the C++ version of this library is currently not
   * supported. Consider submitting a feature request if you really need it.
   */

  /**
   * Returns {@code true} if the map is read-only, {@code false} otherwise.
   */
  public boolean isReadOnly() {
    return Native.isReadOnly(self);
  }

  /**
   * Flushes all data to disk and ensures that the map is stored in consistent state.
   */
  @Override
  public void close() {
    if (self != null) {
      Native.close(self);
      self = null;
    }
  }

  @Override
  protected void finalize() throws Throwable {
    if (self != null) {
      System.err.printf("WARNING %s.finalize() calls close()\n", getClass().getName());
      close();
    }
    super.finalize();
  }

  // -----------------------------------------------------------------------------------------------
  // The following functions are long running operations also available via the command line tool.
  // -----------------------------------------------------------------------------------------------

  /*
   * The method `stats` as provided in the C++ version of this library is currently not supported.
   * Consider submitting a feature request if you really need it.
   */

  /**
   * Imports key-value pairs from an input file or directory into the map located in
   * {@code directory}. If {@code input} refers to a directory all files in that directory will be
   * imported, except hidden files starting with a dot and other sub-directories. A description of
   * the file format can be found in the <a href="http://multimap.io/overview#multimap-import">
   * overview</a> section on the project's website.
   * 
   * <p><b>Acquires</b> a directory lock on directory.</p>
   * 
   * @throws Exception if the constructor of {@link Map} fails or if the input file or directory
   * cannot be read.
   */
  public static void importFromBase64(Path directory, Path input) throws Exception {
    importFromBase64(directory, input, new Options());
  }

  /**
   * Same as {@link #importFromBase64(Path, Path)}, but gives the user more control by providing an
   * {@link Options} parameter which is passed to the constructor of Map when opening. This way a
   * map can be created if it does not already exist.
   */
  public static void importFromBase64(Path directory, Path input, Options options) throws Exception {
    Check.notNull(directory);
    Check.notNull(input);
    Check.notNull(options);
    Native.importFromBase64(directory.toString(), input.toString(), options);
  }

  /**
   * Exports all key-value pairs from the map located in {@code directory} to a file denoted by
   * {@code output}. If the file already exists, its content will be overwritten. The generated
   * file is in canonical form as described in the
   * <a href="http://multimap.io/overview#multimap-import">overview</a> section on the project's
   * website.
   * 
   * <p><b>Acquires</b> a directory lock on directory.</p>
   * 
   * @throws Exception if one of the following is true:
   * <ul>
   * <li>the directory does not exist</li>
   * <li>the directory cannot be locked</li>
   * <li>the directory does not contain a map</li>
   * <li>the creation of the output file failed</li>
   * </ul>
   */
  public static void exportToBase64(Path directory, Path output) throws Exception {
    exportToBase64(directory, output, new Options());
  }
  
  /**
   * Same as {@link #exportToBase64(Path, Path)}, but gives the user more control by providing an
   * {@link Options} parameter. Most users will use this to pass a compare function that triggers a
   * sorting of all lists before exporting them.
   */
  public static void exportToBase64(Path directory, Path output, Options options) throws Exception {
    Check.notNull(directory);
    Check.notNull(output);
    Check.notNull(options);
    Native.exportToBase64(directory.toString(), output.toString(), options);
  }

  /**
   * Rewrites the map located in {@code directory} to the directory denoted by {@code output}
   * performing various optimizations. For more details please refer to the
   * <a href="http://multimap.io/overview#multimap-optimize">overview</a> section on the project's
   * website.
   * 
   * <p><b>Acquires</b> a directory lock on directory.</p>
   * 
   * @throws Exception if one of the following is true:
   * <ul>
   * <li>the directory does not exist</li>
   * <li>the directory cannot be locked</li>
   * <li>the directory does not contain a map</li>
   * <li>the creation of a new map in {@code output} failed</li>
   * </ul>
   */
  public static void optimize(Path directory, Path output) throws Exception {
    Options options = new Options();
    options.keepBlockSize();
    options.keepNumPartitions();
    optimize(directory, output, options);
  }
  
  /**
   * Same as {@link #optimize(Path, Path)}, but gives the user more control by providing an
   * {@link Options} parameter. Most users will use this to pass a compare function that triggers a
   * sorting of all lists.
   */
  public static void optimize(Path directory, Path output, Options options) throws Exception {
    Check.notNull(directory);
    Check.notNull(output);
    Check.notNull(options);
    Native.optimize(directory.toString(), output.toString(), options);
  }

  private static class Native {
    static native ByteBuffer newMap(String directory, Options options) throws Exception;
    static native void put(ByteBuffer self, byte[] key, byte[] value) throws Exception;
    static native ByteBuffer get(ByteBuffer self, byte[] key);
    static native boolean containsKey(ByteBuffer self, byte[] key);
    static native boolean removeKey(ByteBuffer self, byte[] key);
    static native long removeKeys(ByteBuffer self, Predicate predicate);
    static native boolean removeValue(ByteBuffer self, byte[] key, byte[] value);
    static native boolean removeValue(ByteBuffer self, byte[] key, Predicate predicate);
    static native long removeValues(ByteBuffer self, byte[] key, byte[] value);
    static native long removeValues(ByteBuffer self, byte[] key, Predicate predicate);
    static native boolean replaceValue(ByteBuffer self, byte[] key, byte[] oldValue, byte[] newValue);
    static native boolean replaceValue(ByteBuffer self, byte[] key, Function function);
    static native long replaceValues(ByteBuffer self, byte[] key, byte[] oldValue, byte[] newValue);
    static native long replaceValues(ByteBuffer self, byte[] key, Function function);
    static native void forEachKey(ByteBuffer self, Procedure action);
    static native void forEachValue(ByteBuffer self, byte[] key, Procedure action);
    static native boolean isReadOnly(ByteBuffer self);
    static native void close(ByteBuffer self);
    static native void importFromBase64(String directory, String input, Options options) throws Exception;
    static native void exportToBase64(String directory, String output, Options options) throws Exception;
    static native void optimize(String directory, String output, Options options) throws Exception;
  }
}
