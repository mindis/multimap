/*
 * This file is part of the Multimap library.  http://multimap.io
 *
 * Copyright (C) 2015  Martin Trenkmann
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
import java.nio.file.Path;

/**
 * This class implements a 1:n key-value store where each key is associated with a list of values.
 * 
 * @author Martin Trenkmann
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

  static class Limits {

    /**
     * Returns the maximum size, in number of bytes, of a key to put.
     */
    static int maxKeySize() {
      return Native.maxKeySize();
    }

    /**
     * Returns the maximum size, in number of bytes, of a value to put.
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
   * Opens a map in {@code directory}.
   * 
   * @throws Exception if the directory or the map inside does not exist.
   */
  public Map(Path directory) throws Exception {
    this(directory, new Options());
  }
  
  /**
   * Opens or creates a map in {@code directory}. The directory must already exist. See
   * {@link Options} for more information.
   * 
   * @throws Exception if the directory does not exist or other conditions depending on
   *         {@code options} are not met.
   */
  public Map(Path directory, Options options) throws Exception {
    this(Native.newMap(directory.toString(), options));
  }

  /**
   * Appends {@code value} to the end of the list associated with {@code key}.
   * 
   * @throws Exception if {@code key} or {@code value} are too big. See {@link Limits#maxKeySize()}
   *         and {@link Limits#maxValueSize()} for more information.
   */
  public void put(byte[] key, byte[] value) throws Exception {
    Check.notNull(key);
    Check.notNull(value);
    Native.put(self, key, value);
  }

  /**
   * Returns an iterator to the list associated with {@code key}. If the key does not exist an empty
   * iterator is returned to indicate an empty list. If the returned iterator is not needed anymore
   * {@link Iterator#close()} must be called in order to release the reader lock.
   * 
   * <p>Never write code like this, because it does not close the iterator:<ul>
   * <li><code>boolean contains = map.get(key).hasNext();</code></li>
   * <li><code>long numValues = map.get(key).available();</code></li>
   * </ul></p>
   */
  @SuppressWarnings("resource")
  public Iterator get(byte[] key) {
    Check.notNull(key);
    ByteBuffer nativePtr = Native.get(self, key);
    return (nativePtr != null) ? new Iterator(nativePtr) : Iterator.EMPTY;
  }
  
  /**
   * Returns {@code true} if this map contains a mapping for the specified key.
   */
  public boolean containsKey(byte[] key) {
    Check.notNull(key);
    return Native.containsKey(self, key);
  }

  /**
   * Removes all values in the list associated with {@code key}.
   * 
   * @return {@code true} if a key was removed, {@code false} otherwise.
   */
  public boolean removeKey(byte[] key) {
    Check.notNull(key);
    return Native.removeKey(self, key);
  }

  public long removeKeys(Predicate predicate) {
    return Native.removeKeys(self, predicate);
  }

  /**
   * Removes the first value in the list associated with {@code key} which is equal to {@code value}
   * .
   * 
   * @return {@code true} if a value was removed, {@code false} otherwise.
   */
  public boolean removeValue(byte[] key, byte[] value) {
    Check.notNull(key);
    Check.notNull(value);
    return Native.removeValue(self, key, value);
  }

  /**
   * Removes the first value in the list associated with {@code key} for which {@code predicate}
   * yields {@code true}.
   * 
   * @return {@code true} if a value was removed, {@code false} otherwise.
   */
  public boolean removeValue(byte[] key, Predicate predicate) {
    Check.notNull(key);
    Check.notNull(predicate);
    return Native.removeValue(self, key, predicate);
  }

  /**
   * Removes all values in the list associated with {@code key} that are equal to {@code value}.
   * 
   * @return The number of removed values.
   */
  public long removeValues(byte[] key, byte[] value) {
    Check.notNull(key);
    Check.notNull(value);
    return Native.removeValues(self, key, value);
  }

  /**
   * Removes all values in the list associated with {@code key} for which {@code predicate} yields
   * {@code true}.
   * 
   * @return The number of removed values.
   */
  public long removeValues(byte[] key, Predicate predicate) {
    Check.notNull(key);
    Check.notNull(predicate);
    return Native.removeValues(self, key, predicate);
  }

  /**
   * Replaces the first value in the list associated with {@code key} which is equal to
   * {@code oldValue} by {@code newValue}. The replacement does not happen in-place. Instead, the
   * old value is marked as removed and the new value is appended to the end of the list.
   * 
   * @return {@code true} if a value was replaced, {@code false} otherwise.
   */
  public boolean replaceValue(byte[] key, byte[] oldValue, byte[] newValue) {
    Check.notNull(key);
    Check.notNull(oldValue);
    Check.notNull(newValue);
    return Native.replaceValue(self, key, oldValue, newValue);
  }

  /**
   * Replaces the first value in the list associated with {@code key} by the result of invoking
   * {@code map}. If the result of {@code map} is {@code null} no replacement is performed. The
   * replacement does not happen in-place. Instead, the old value is marked as removed and the new
   * value is appended to the end of the list.
   * 
   * @return {@code true} if a value was replaced, {@code false} otherwise.
   */
  public boolean replaceValue(byte[] key, Function map) {
    Check.notNull(key);
    Check.notNull(map);
    return Native.replaceValue(self, key, map);
  }

  /**
   * Replaces all values in the list associated with {@code key} which are equal to {@code oldValue}
   * by {@code newValue}. A replacement does not happen in-place. Instead, the old value is marked
   * as removed and the new value is appended to the end of the list.
   * 
   * @return The number of replaced values.
   */
  public long replaceValues(byte[] key, byte[] oldValue, byte[] newValue) {
    Check.notNull(key);
    Check.notNull(oldValue);
    Check.notNull(newValue);
    return Native.replaceValues(self, key, oldValue, newValue);
  }

  /**
   * Replaces all values in the list associated with {@code key} by the result of invoking
   * {@code map}. If the result of {@code map} is {@code null} no replacement is performed. A
   * replacement does not happen in-place. Instead, the old value is marked as removed and the new
   * value is appended to the end of the list.
   * 
   * @return The number of replaced values.
   */
  public long replaceValues(byte[] key, Function map) {
    Check.notNull(key);
    Check.notNull(map);
    return Native.replaceValues(self, key, map);
  }

  /**
   * Applies {@code process} to each key of the map whose associated list is not empty. For the time
   * of execution the entire map is locked for read-only operations. It is possible to keep a
   * reference to the map itself within {@code process} and to call other read-only operations such
   * as {@link #get(byte[])}. However, trying to call mutable operations such as
   * {@link #getMutable(byte[])} will cause a deadlock.
   */
  public void forEachKey(Procedure process) {
    Check.notNull(process);
    Native.forEachKey(self, process);
  }

  /**
   * Applies {@code process} to each value in the list associated with {@code key}. This is a
   * shorthand for requesting a read-only iterator via {@link #get(byte[])} followed by an
   * application of {@code process} to each value obtained via {@link Iterator#next()}.
   */
  public void forEachValue(byte[] key, Procedure process) {
    Check.notNull(process);
    Native.forEachValue(self, key, process);
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
   * Imports key-value pairs from Base64-encoded text file(s) denoted by {@code input} into the map
   * located in the directory denoted by {@code directory}. If {@code input} is a directory all
   * contained files will we imported recursively.
   * 
   * @throws Exception if the directory or file does not exist, or the map is already in use.
   */
  public static void importFromBase64(Path directory, Path input) throws Exception {
    importFromBase64(directory, input, new Options());
  }

  /**
   * Same as {@link #importFromBase64(Path, Path)} but allows further configuration.
   */
  public static void importFromBase64(Path directory, Path input, Options options) throws Exception {
    Check.notNull(directory);
    Check.notNull(input);
    Check.notNull(options);
    Native.importFromBase64(directory.toString(), input.toString(), options);
  }

  /**
   * Exports all key-value pairs from the map located in the directory denoted by {@code directory}
   * to a Base64-encoded text file denoted by {@code output}. If the file already exists its content
   * will be overwritten.
   * 
   * @throws Exception if the directory does not exist, or the map is already in use.
   */
  public static void exportToBase64(Path directory, Path output) throws Exception {
    exportToBase64(directory, output, new Options());
  }

  public static void exportToBase64(Path directory, Path output, Options options) throws Exception {
    Check.notNull(directory);
    Check.notNull(output);
    Check.notNull(options);
    Native.exportToBase64(directory.toString(), output.toString(), options);
  }

  /**
   * Optimizes a map performing the following tasks:
   * <ul>
   * <li>Defragmentation. All blocks which belong to the same list are written sequentially to disk
   * which improves locality and leads to better read performance.</li>
   * <li>Garbage collection. Values marked as removed will not be copied which reduces the size of
   * the map and also improves locality.</li>
   * </ul>
   * 
   * @param source <ul>
   *        <li>Must refer to an existing directory containing a map.</li>
   *        </ul>
   * 
   * @param target <ul>
   *        <li>Must refer to an existing directory that does not contain a map.</li>
   *        </ul>
   * 
   * @throws Exception if any of the following happens:
   *         <ul>
   *         <li>Opening a map in {@code source} failed.</li>
   *         <li>Creating a map in {@code target} failed.</li>
   *         <li>A runtime error, such as running out of disk space, occurred.</li>
   *         </ul>
   */
  public static void optimize(Path source, Path target) throws Exception {
    Options options = new Options();
    options.keepBlockSize();
    options.keepNumShards();
    optimize(source, target, options);
  }
  
  /**
   * Optimizes a map performing the following tasks:
   * <ul>
   * <li>Defragmentation. All blocks which belong to the same list are written sequentially to disk
   * which improves locality and leads to better read performance.</li>
   * <li>Garbage collection. Values marked as removed will not be copied which reduces the size of
   * the map and also improves locality.</li>
   * </ul>
   * 
   * @param source <ul>
   *        <li>Must refer to an existing directory containing a map.</li>
   *        </ul>
   * 
   * @param target <ul>
   *        <li>Must refer to an existing directory that does not contain a map.</li>
   *        </ul>
   * 
   * @param options <ul>
   *        <li>Call {@code options.keepNumShards()} to leave the number of shards unchanged.</li>
   *        <li>Call {@code options.keepBlockSize()} to leave the block size unchanged.</li>
   *        <li>{@code options.isCreateIfMissing()} is implicitly {@code true} and can be ignored.</li>
   *        <li>{@code options.isErrorIfExists()} is implicitly {@code true} and can be ignored.</li>
   *        <li>{@code options.isReadOnly()} is ignored.</li>
   *        </ul>
   * 
   * @throws Exception if any of the following happens:
   *         <ul>
   *         <li>Opening a map in {@code source} failed.</li>
   *         <li>Creating a map in {@code target} failed.</li>
   *         <li>A runtime error, such as running out of disk space, occurred.</li>
   *         </ul>
   */
  public static void optimize(Path source, Path target, Options options) throws Exception {
    Check.notNull(source);
    Check.notNull(target);
    Check.notNull(options);
    Native.optimize(source.toString(), target.toString(), options);
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
    static native void optimize(String source, String target, Options options) throws Exception;
  }
}
