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

  /**
   * An {@link Iterator} that iterates a list of values in read-only mode. The optional
   * {@link #remove()} operation will throw an {@link UnsupportedOperationException} if invoked.
   * Objects of this class hold a reader lock on the associated list so that multiple threads can
   * iterate the list at the same time, but no thread can modify it. If the iterator is not needed
   * anymore {@link #close()} must be called in order to release the reader lock.
   */
  static class ListIterator extends Iterator {

    private ByteBuffer self;

    private ListIterator() {}

    private ListIterator(ByteBuffer nativePtr) {
      Check.notNull(nativePtr);
      Check.notZero(nativePtr.capacity());
      self = nativePtr;
    }

    @Override
    public long available() {
      return Native.available(self);
    }

    @Override
    public boolean hasNext() {
      return Native.hasNext(self);
    }

    @Override
    public ByteBuffer next() {
      return Native.next(self).asReadOnlyBuffer();
    }

    @Override
    public ByteBuffer peekNext() {
      return Native.peekNext(self).asReadOnlyBuffer();
    }

    @Override
    public void remove() {
      throw new UnsupportedOperationException();
    }

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

    static class Native {
      static native long available(ByteBuffer self);
      static native boolean hasNext(ByteBuffer self);
      static native ByteBuffer next(ByteBuffer self);
      static native ByteBuffer peekNext(ByteBuffer self);
      static native void close(ByteBuffer self);
    }

    /**
     * A constant that represents an empty iterator.
     */
    public static final ListIterator EMPTY = new ListIterator() {

      @Override
      public long available() {
        return 0;
      }

      @Override
      public boolean hasNext() {
        return false;
      }

      @Override
      public ByteBuffer next() {
        return null;
      }

      @Override
      public ByteBuffer peekNext() {
        return null;
      }
    };
  }

  /**
   * An {@link Iterator} that iterates a list of values in read-write mode. Objects of this class
   * hold a writer lock on the associated list to ensure exclusive access to it. Other threads
   * trying to acquire a reader or writer lock on the same list will block until the lock is
   * released via {@link #close()}. If the iterator is not needed anymore {@link #close()} must be
   * called in order to release the writer lock.
   */
  static class MutableListIterator extends Iterator {

    private ByteBuffer self;

    private MutableListIterator() {}

    private MutableListIterator(ByteBuffer nativePtr) {
      Check.notNull(nativePtr);
      Check.notZero(nativePtr.capacity());
      self = nativePtr;
    }

    @Override
    public long available() {
      return Native.available(self);
    }

    @Override
    public boolean hasNext() {
      return Native.hasNext(self);
    }

    @Override
    public ByteBuffer next() {
      return Native.next(self).asReadOnlyBuffer();
    }

    @Override
    public ByteBuffer peekNext() {
      return Native.peekNext(self).asReadOnlyBuffer();
    }

    @Override
    public void remove() {
      Native.remove(self);
    }

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

    static class Native {
      static native long available(ByteBuffer self);
      static native boolean hasNext(ByteBuffer self);
      static native ByteBuffer next(ByteBuffer self);
      static native ByteBuffer peekNext(ByteBuffer self);
      static native void remove(ByteBuffer self);
      static native void close(ByteBuffer self);
    }
  }

  private ByteBuffer self;

  private Map(ByteBuffer nativePtr) {
    Check.notNull(nativePtr);
    Check.notZero(nativePtr.capacity());
    self = nativePtr;
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
   * @throws Exception if {@code key} or {@code value} are too big. See
   *         {@link Limits#maxKeySize()} and {@link Limits#maxValueSize()} for more
   *         information.
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
   */
  @SuppressWarnings("resource")
  public Iterator get(byte[] key) {
    Check.notNull(key);
    ByteBuffer nativePtr = Native.get(self, key);
    return (nativePtr != null) ? new ListIterator(nativePtr) : Iterator.EMPTY;
  }

  /**
   * Returns a mutable iterator to the list associated with {@code key}. If the key does not exist
   * an empty iterator is returned to indicate an empty list. If the returned iterator is not needed
   * anymore {@link Iterator#close()} must be called in order to release the writer lock.
   */
  @SuppressWarnings("resource")
  public Iterator getMutable(byte[] key) {
    Check.notNull(key);
    ByteBuffer nativePtr = Native.getMutable(self, key);
    return (nativePtr != null) ? new MutableListIterator(nativePtr) : Iterator.EMPTY;
  }

  /**
   * Removes all values in the list associated with {@code key}.
   * 
   * @return The number of removed values.
   */
  public long remove(byte[] key) {
    Check.notNull(key);
    return Native.remove(self, key);
  }

  /**
   * Removes all values in the list associated with {@code key} for which {@code predicate} yields
   * {@code true}.
   * 
   * @return The number of removed values.
   */
  public long removeAll(byte[] key, Predicate predicate) {
    Check.notNull(key);
    Check.notNull(predicate);
    return Native.removeAll(self, key, predicate);
  }

  /**
   * Removes all values in the list associated with {@code key} that are equal to {@code value}.
   * 
   * @return The number of removed values.
   */
  public long removeAllEqual(byte[] key, byte[] value) {
    Check.notNull(key);
    Check.notNull(value);
    return Native.removeAllEqual(self, key, value);
  }

  /**
   * Removes the first value in the list associated with {@code key} for which {@code predicate}
   * yields {@code true}.
   * 
   * @return {@code true} if a value was removed, {@code false} otherwise.
   */
  public boolean removeFirst(byte[] key, Predicate predicate) {
    Check.notNull(key);
    Check.notNull(predicate);
    return Native.removeFirst(self, key, predicate);
  }

  /**
   * Removes the first value in the list associated with {@code key} which is equal to
   * {@code value}.
   * 
   * @return {@code true} if a value was removed, {@code false} otherwise.
   */
  public boolean removeFirstEqual(byte[] key, byte[] value) {
    Check.notNull(key);
    Check.notNull(value);
    return Native.removeFirstEqual(self, key, value);
  }

  /**
   * Replaces all values in the list associated with {@code key} by the result of invoking
   * {@code function}. If the result of {@code function} is {@code null} no replacement is
   * performed. A replacement does not happen in-place. Instead, the old value is marked as removed
   * and the new value is appended to the end of the list.
   * 
   * @return The number of replaced values.
   */
  public long replaceAll(byte[] key, Function function) {
    Check.notNull(key);
    Check.notNull(function);
    return Native.replaceAll(self, key, function);
  }

  /**
   * Replaces all values in the list associated with {@code key} which are equal to {@code oldValue}
   * by {@code newValue}. A replacement does not happen in-place. Instead, the old value is marked
   * as removed and the new value is appended to the end of the list.
   * 
   * @return The number of replaced values.
   */
  public long replaceAllEqual(byte[] key, byte[] oldValue, byte[] newValue) {
    Check.notNull(key);
    Check.notNull(oldValue);
    Check.notNull(newValue);
    return Native.replaceAllEqual(self, key, oldValue, newValue);
  }

  /**
   * Replaces the first value in the list associated with {@code key} by the result of invoking
   * {@code function}. If the result of {@code function} is {@code null} no replacement is
   * performed. The replacement does not happen in-place. Instead, the old value is marked as
   * removed and the new value is appended to the end of the list.
   * 
   * @return {@code true} if a value was replaced, {@code false} otherwise.
   */
  public boolean replaceFirst(byte[] key, Function function) {
    Check.notNull(key);
    Check.notNull(function);
    return Native.replaceFirst(self, key, function);
  }

  /**
   * Replaces the first value in the list associated with {@code key} which is equal to
   * {@code oldValue} by {@code newValue}. The replacement does not happen in-place. Instead, the
   * old value is marked as removed and the new value is appended to the end of the list.
   * 
   * @return {@code true} if a value was replaced, {@code false} otherwise.
   */
  public boolean replaceFirstEqual(byte[] key, byte[] oldValue, byte[] newValue) {
    Check.notNull(key);
    Check.notNull(oldValue);
    Check.notNull(newValue);
    return Native.replaceFirstEqual(self, key, oldValue, newValue);
  }

  /**
   * Applies {@code action} to each key of the map whose associated list is not empty. For the time
   * of execution the entire map is locked for read-only operations. It is possible to keep a
   * reference to the map itself within {@code action} and to call other read-only operations such
   * as {@link #get(byte[])}. However, trying to call mutable operations such as
   * {@link #getMutable(byte[])} will cause a deadlock.
   */
  public void forEachKey(Procedure action) {
    Check.notNull(action);
    Native.forEachKey(self, action);
  }

  /**
   * Applies {@code action} to each value in the list associated with {@code key}. This is a
   * shorthand for requesting a read-only iterator via {@link #get(byte[])} followed by an
   * application of {@code action} to each value obtained via {@link Iterator#next()}.
   */
  public void forEachValue(byte[] key, Procedure action) {
    Check.notNull(action);
    Native.forEachValue(self, key, action);
  }

  /**
   * Applies {@code action} to each value in the list associated with {@code key} until
   * {@code action} yields {@code false}. This is a shorthand for requesting a read-only iterator
   * via {@link #get(byte[])} followed by an application of {@code action} to each value obtained
   * via {@link Iterator#next()} until {@code action} yields {@code false}.
   */
  public void forEachValue(byte[] key, Predicate action) {
    Check.notNull(action);
    Native.forEachValue(self, key, action);
  }

  /* The method `forEachEntry` as provided in the C++ version of this library is currently not
   * supported. Consider submitting a feature request if you really need it.
   */

  /* The method `getStats` as provided in the C++ version of this library is currently not
   * supported. Consider submitting a feature request if you really need it.
   */

  /* The method `getTotalStats` as provided in the C++ version of this library is currently not
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

  /* The method `stats` as provided in the C++ version of this library is currently not supported.
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
   * @param source
   *        <ul>
   *        <li>Must refer to an existing directory containing a map.</li>
   *        </ul>
   * 
   * @param target
   *        <ul>
   *        <li>Must refer to an existing directory that does not contain a map.</li>
   *        </ul>
   * 
   * @param options
   *        <ul>
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

  static class Native {
    static native ByteBuffer newMap(String directory, Options options) throws Exception;
    static native void put(ByteBuffer self, byte[] key, byte[] value) throws Exception;
    static native ByteBuffer get(ByteBuffer self, byte[] key);
    static native ByteBuffer getMutable(ByteBuffer self, byte[] key);
    static native long remove(ByteBuffer self, byte[] key);
    static native long removeAll(ByteBuffer self, byte[] key, Predicate predicate);
    static native long removeAllEqual(ByteBuffer self, byte[] key, byte[] value);
    static native boolean removeFirst(ByteBuffer self, byte[] key, Predicate predicate);
    static native boolean removeFirstEqual(ByteBuffer self, byte[] key, byte[] value);
    static native long replaceAll(ByteBuffer self, byte[] key, Function function);
    static native long replaceAllEqual(ByteBuffer self, byte[] key, byte[] oldValue, byte[] newValue);
    static native boolean replaceFirst(ByteBuffer self, byte[] key, Function function);
    static native boolean replaceFirstEqual(ByteBuffer self, byte[] key, byte[] oldValue, byte[] newValue);
    static native void forEachKey(ByteBuffer self, Procedure action);
    static native void forEachValue(ByteBuffer self, byte[] key, Procedure action);
    static native void forEachValue(ByteBuffer self, byte[] key, Predicate action);
    static native boolean isReadOnly(ByteBuffer self);
    static native void close(ByteBuffer self);
    static native void importFromBase64(String directory, String input, Options options) throws Exception;
    static native void exportToBase64(String directory, String output, Options options) throws Exception;
    static native void optimize(String source, String target, Options options) throws Exception;
  }
}
