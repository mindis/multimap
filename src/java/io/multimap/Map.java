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
import io.multimap.Callables.LessThan;
import io.multimap.Callables.Predicate;
import io.multimap.Callables.Procedure;

import java.nio.ByteBuffer;
import java.nio.file.Path;
import java.util.TreeMap;

/**
 * This class represents a 1:n key-value store. In such a store each key is associated with a list
 * of values rather than only a single one. Values can be appended to or removed from a list, and
 * iterated quickly.
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

  /**
   * An {@link Iterator} that iterates a list of values in read-only mode. The optional
   * {@link #deleteValue()} operation will throw an {@link UnsupportedOperationException} if
   * invoked. Objects of this class hold a reader lock on the associated list so that multiple
   * threads can iterate the list at the same time in read-only mode. Remember to call
   * {@link #close()} if the iterator is not needed anymore.
   */
  static class ImmutableListIter extends Iterator {

    private ByteBuffer self;

    private ImmutableListIter(ByteBuffer nativePtr) {
      Check.notNull(nativePtr);
      Check.notZero(nativePtr.capacity());
      self = nativePtr;
    }

    @Override
    public long numValues() {
      return Native.numValues(self);
    }

    @Override
    public void seekToFirst() {
      Native.seekToFirst(self);
    }

    @Override
    public void seekTo(byte[] target) {
      Check.notNull(target);
      Native.seekTo(self, target);
    }

    @Override
    public void seekTo(Predicate predicate) {
      Check.notNull(predicate);
      Native.seekTo(self, predicate);
    }

    @Override
    public boolean hasValue() {
      return Native.hasValue(self);
    }

    @Override
    public ByteBuffer getValue() {
      return Native.getValue(self).asReadOnlyBuffer();
    }

    @Override
    public void deleteValue() {
      throw new UnsupportedOperationException();
    }

    @Override
    public void next() {
      Native.next(self);
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
        System.err.printf(
            "WARNING %s.finalize() calls %s.close(), but should be called by the client.\n",
            getClass().getName());
        close();
      }
      super.finalize();
    }

    static class Native {
      static native long numValues(ByteBuffer self);

      static native void seekToFirst(ByteBuffer self);

      static native void seekTo(ByteBuffer self, byte[] target);

      static native void seekTo(ByteBuffer self, Predicate predicate);

      static native boolean hasValue(ByteBuffer self);

      static native ByteBuffer getValue(ByteBuffer self);

      static native void next(ByteBuffer self);

      static native void close(ByteBuffer self);
    }
  }

  /**
   * An {@link Iterator} that iterates a list of values in read-write mode. Objects of this class
   * hold a writer lock on the associated list that ensures exclusive access to it. Other threads
   * trying to acquire a reader or writer lock on the same list will block until the lock is
   * released via {@link #close()}.
   */
  static class MutableListIter extends Iterator {

    private ByteBuffer self;

    private MutableListIter(ByteBuffer nativePtr) {
      Check.notNull(nativePtr);
      Check.notZero(nativePtr.capacity());
      self = nativePtr;
    }

    @Override
    public long numValues() {
      return Native.numValues(self);
    }

    @Override
    public void seekToFirst() {
      Native.seekToFirst(self);
    }

    @Override
    public void seekTo(byte[] target) {
      Check.notNull(target);
      Native.seekTo(self, target);
    }

    @Override
    public void seekTo(Predicate predicate) {
      Check.notNull(predicate);
      Native.seekTo(self, predicate);
    }

    @Override
    public boolean hasValue() {
      return Native.hasValue(self);
    }

    @Override
    public ByteBuffer getValue() {
      return Native.getValue(self).asReadOnlyBuffer();
    }

    @Override
    public void deleteValue() {
      Native.deleteValue(self);
    }

    @Override
    public void next() {
      Native.next(self);
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
        System.err.printf(
            "WARNING %s.finalize() calls %s.close(), but should be called by the client.\n",
            getClass().getName());
        close();
      }
      super.finalize();
    }

    static class Native {
      static native long numValues(ByteBuffer self);

      static native void seekToFirst(ByteBuffer self);

      static native void seekTo(ByteBuffer self, byte[] target);

      static native void seekTo(ByteBuffer self, Predicate predicate);

      static native boolean hasValue(ByteBuffer self);

      static native ByteBuffer getValue(ByteBuffer self);

      static native void deleteValue(ByteBuffer self);

      static native void next(ByteBuffer self);

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
    this(Native.open(directory.toString(), options.toString()));
  }

  /**
   * Flushes all data to disk and ensures that the map is stored in consistent state.
   */
  public void close() {
    if (self != null) {
      Native.close(self);
      self = null;
    }
  }

  /**
   * Appends {@code value} to the end of the list associated with {@code key}.
   * 
   * @throws Exception if {@code key} or {@code value} are too long. See {@link #maxKeySize()} and
   *         {@link #maxValueSize()} for more information.
   */
  public void put(byte[] key, byte[] value) throws Exception {
    Check.notNull(key);
    Check.notNull(value);
    Native.put(self, key, value);
  }

  /**
   * Returns an immutable iterator to the list associated with {@code key}. This method always
   * returns an iterator, and never {@code null}, even if the key does not exist. In this case the
   * list is considered to be empty. Remember to call {@link Iterator#close()} if the iterator is
   * not needed anymore to release the shared lock on the underlying list.
   */
  @SuppressWarnings("resource")
  public Iterator get(byte[] key) {
    Check.notNull(key);
    ByteBuffer nativePtr = Native.get(self, key);
    return (nativePtr != null) ? new ImmutableListIter(nativePtr) : Iterator.EMPTY;
  }

  /**
   * Returns a mutable iterator to the list associated with {@code key}. This method always returns
   * an iterator, and never {@code null}, even if the key does not exist. In this case the list is
   * considered to be empty. Remember to call {@link Iterator#close()} if the iterator is not needed
   * anymore to release the exclusive lock on the underlying list.
   */
  @SuppressWarnings("resource")
  public Iterator getMutable(byte[] key) {
    Check.notNull(key);
    ByteBuffer nativePtr = Native.getMutable(self, key);
    return (nativePtr != null) ? new MutableListIter(nativePtr) : Iterator.EMPTY;
  }

  /**
   * Tells whether the list associated with {@code key} contains at least one value. If the key does
   * not exist the list is considered to be empty.
   */
  public boolean contains(byte[] key) {
    return Native.contains(self, key);
  }

  /**
   * Deletes all values for {@code key} by clearing the associated list.
   * 
   * @return The number of deleted values.
   */
  public long delete(byte[] key) {
    Check.notNull(key);
    return Native.delete(self, key);
  }

  /**
   * Deletes the first value in the list associated with {@code key} for which {@code predicate}
   * returns {@code true}.
   * 
   * @return {@code true} if a value was deleted, {@code false} otherwise.
   */
  public boolean deleteFirst(byte[] key, Predicate predicate) {
    Check.notNull(key);
    Check.notNull(predicate);
    return Native.deleteFirst(self, key, predicate);
  }

  /**
   * Deletes all values in the list associated with {@code key} which are equal to {@code value}.
   * Two values compare equal if their byte arrays match.
   * 
   * @return {@code true} if a value was deleted, {@code false} otherwise.
   */
  public boolean deleteFirstEqual(byte[] key, byte[] value) {
    Check.notNull(key);
    Check.notNull(value);
    return Native.deleteFirstEqual(self, key, value);
  }

  /**
   * Deletes all values in the list associated with {@code key} for which {@code predicate} returns
   * {@code true}.
   * 
   * @return The number of deleted values.
   */
  public long deleteAll(byte[] key, Predicate predicate) {
    Check.notNull(key);
    Check.notNull(predicate);
    return Native.deleteAll(self, key, predicate);
  }

  /**
   * Deletes all values in the list associated with {@code key} that are equal to {@code value}. Two
   * values compare equal if their byte arrays match.
   * 
   * @return The number of deleted values.
   */
  public long deleteAllEqual(byte[] key, byte[] value) {
    Check.notNull(key);
    Check.notNull(value);
    return Native.deleteAllEqual(self, key, value);
  }

  /**
   * Replaces the first value in the list associated with {@code key} by the result of invoking
   * {@code function}. The result of {@code function} is ignored as long as it is {@code null}. The
   * replacement does not happen in-place. Instead, the old value is marked as deleted and the new
   * value is appended to the end of the list. Future releases will support in-place replacements.
   * 
   * @return {@code true} if a value was replaced, {@code false} otherwise.
   */
  public boolean replaceFirst(byte[] key, Function function) {
    Check.notNull(key);
    Check.notNull(function);
    return Native.replaceFirst(self, key, function);
  }

  /**
   * Replaces the first occurrence of {@code oldValue} in the list associated with {@code key} by
   * {@code newValue}. The replacement does not happen in-place. Instead, the old value is marked as
   * deleted and the new value is appended to the end of the list. Future releases will support
   * in-place replacements.
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
   * Replaces all values in the list associated with {@code key} by the result of invoking
   * {@code function}. The result of {@code function} is ignored as long as it is {@code null}. The
   * replacement does not happen in-place. Instead, the old value is marked as deleted and the new
   * value is appended to the end of the list. Future releases will support in-place replacements.
   * 
   * @return The number of replaced values.
   */
  public long replaceAll(byte[] key, Function function) {
    Check.notNull(key);
    Check.notNull(function);
    return Native.replaceAll(self, key, function);
  }

  /**
   * Replaces all occurrences of {@code oldValue} in the list associated with {@code key} by
   * {@code newValue}. The replacements do not happen in-place. Instead, the old value is marked as
   * deleted and the new value is appended to the end of the list. Future releases will support
   * in-place replacements.
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
   * Applies {@code procedure} to each key of the map whose associated list is not empty. For the
   * time of execution the entire map is locked for read-only operations. It is possible to keep a
   * reference to the map within {@code procedure} and to call other read-only operations such as
   * {@link #get(byte[])}. However, trying to call mutable operations such as
   * {@link #getMutable(byte[])} will cause a deadlock.
   */
  public void forEachKey(Procedure procedure) {
    Check.notNull(procedure);
    Native.forEachKey(self, procedure);
  }

  /**
   * Applies {@code procedure} to each value associated with {@code key}. This is a shorthand for
   * {@link #get(byte[])} followed by an application of {@code procedure} to each value obtained via
   * {@link Iterator#getValue()}.
   */
  public void forEachValue(byte[] key, Procedure procedure) {
    Check.notNull(procedure);
    Native.forEachValue(self, key, procedure);
  }

  /**
   * Applies {@code predicate} to each value associated with {@code key} until {@code predicate}
   * returns {@code false}. This is a shorthand for {@link #get(byte[])} followed by an application
   * of {@code predicate} to each value obtained via {@link Iterator#getValue()} until
   * {@code predicate} returns {@code false}.
   */
  public void forEachValue(byte[] key, Predicate predicate) {
    Check.notNull(predicate);
    Native.forEachValue(self, key, predicate);
  }

  /**
   * Collects and returns current properties of the map. For the time of execution the map is locked
   * so that all other operations will block.
   */
  public java.util.Map<String, String> getProperties() {
    java.util.Map<String, String> properties = new TreeMap<>();
    String[] lines = Native.getProperties(self).split("\n");
    for (String line : lines) {
      String[] property = line.split("=");
      properties.put(property[0], property[1]);
    }
    return properties;
  }

  /**
   * Returns the maximum size, in number of bytes, of a key to put. Currently this value is
   * {@code 65536}.
   */
  public int maxKeySize() {
    return Native.maxKeySize(self);
  }

  /**
   * Returns the maximum size, in number of bytes, of a value to put. Currently this value is
   * {@code options.getBlockSize()-2}.
   */
  public int maxValueSize() {
    return Native.maxValueSize(self);
  }

  @Override
  protected void finalize() throws Throwable {
    if (self != null) {
      System.err.printf(
          "WARNING %s.finalize() calls close(), but should be called by the client.\n", getClass()
              .getName());
      close();
    }
    super.finalize();
  }

  /**
   * Copies a map from one directory into another performing optimization tasks. Both directories
   * must exist and the source map must not be open. This operation may be a long running process
   * depending on the size of the map. The tasks performed are:
   * 
   * <ul>
   * <li>Writing all blocks that belong to the same list physically next to each other.</li>
   * <li>Not writing values that are marked as deleted, i.e. removing them physically.</li>
   * </ul>
   * 
   * @throws Exception if any directory does not exist or some error occurred opening the map.
   */
  public static void Optimize(Path from, Path to) throws Exception {
    Optimize(from, to, -1);
  }

  /**
   * Same as {@link #Optimize(Path, Path)} but using {@code newBlockSize} as the block size for the
   * new map. An appropriate block size can lead to a better average load factor of the blocks all
   * lists are composed of, which results in better overall performance.
   */
  public static void Optimize(Path from, Path to, int newBlockSize) throws Exception {
    Optimize(from, to, null, newBlockSize);
  }

  /**
   * Same as {@link #Optimize(Path, Path)} but, in addition, sorting each list before writing it,
   * using {@code lessThan} as the comparing function.
   */
  public static void Optimize(Path from, Path to, LessThan lessThan) throws Exception {
    Optimize(from, to, lessThan, -1);
  }

  /**
   * Combines {@link #Optimize(Path, Path, LessThan)} and {@link #Optimize(Path, Path, int)}.
   */
  public static void Optimize(Path from, Path to, LessThan lessThan, int newBlockSize)
      throws Exception {
    Check.notNull(from);
    Check.notNull(to);
    Native.Optimize(from.toString(), to.toString(), lessThan, newBlockSize);
  }

  /**
   * Imports the content of a base64-encoded text file into a map located in {@code directory}.
   * 
   * @throws Exception if the directory or file does not exist, or the map is already open.
   */
  public static void Import(Path directory, Path file) throws Exception {
    Import(directory, file, false);
  }

  /**
   * Same as {@link #Import(Path, Path)} but creates a new map if it does not yet exist.
   */
  public static void Import(Path directory, Path file, boolean createIfMissing) throws Exception {
    Import(directory, file, createIfMissing, -1);
  }

  /**
   * Same as {@link #Import(Path, Path, boolean)} using {@code blockSize} for the map to be created
   * if this is the case.
   */
  public static void Import(Path directory, Path file, boolean createIfMissing, int blockSize)
      throws Exception {
    Check.notNull(directory);
    Check.notNull(file);
    Native.Import(directory.toString(), file.toString(), createIfMissing, blockSize);
  }

  /**
   * Exports the base64-encoded content of the map located in {@code directory} to text file.
   * 
   * @throws Exception if the directory does not exist, or the map is already open.
   */
  public static void Export(Path directory, Path file) throws Exception {
    Check.notNull(directory);
    Check.notNull(file);
    Native.Export(directory.toString(), file.toString());
  }

  static class Native {
    static native ByteBuffer open(String directory, String options) throws Exception;

    static native void close(ByteBuffer self);

    static native void put(ByteBuffer self, byte[] key, byte[] value) throws Exception;

    static native ByteBuffer get(ByteBuffer self, byte[] key);

    static native ByteBuffer getMutable(ByteBuffer self, byte[] key);

    static native boolean contains(ByteBuffer self, byte[] key);

    static native long delete(ByteBuffer self, byte[] key);

    static native boolean deleteFirst(ByteBuffer self, byte[] key, Predicate predicate);

    static native boolean deleteFirstEqual(ByteBuffer self, byte[] key, byte[] value);

    static native long deleteAll(ByteBuffer self, byte[] key, Predicate predicate);

    static native long deleteAllEqual(ByteBuffer self, byte[] key, byte[] value);

    static native boolean replaceFirst(ByteBuffer self, byte[] key, Function function);

    static native boolean replaceFirstEqual(ByteBuffer self, byte[] key, byte[] oldValue,
        byte[] newValue);

    static native long replaceAll(ByteBuffer self, byte[] key, Function function);

    static native long replaceAllEqual(ByteBuffer self, byte[] key, byte[] oldValue, byte[] newValue);

    static native void forEachKey(ByteBuffer self, Procedure procedure);

    static native void forEachValue(ByteBuffer self, byte[] key, Procedure procedure);

    static native void forEachValue(ByteBuffer self, byte[] key, Predicate predicate);

    static native String getProperties(ByteBuffer self);

    static native int maxKeySize(ByteBuffer self);

    static native int maxValueSize(ByteBuffer self);

    static native void Optimize(String from, String to, LessThan lessThan, int newBlockSize)
        throws Exception;

    static native void Import(String directory, String file, boolean createIfMissing, int blockSize)
        throws Exception;

    static native void Export(String directory, String file) throws Exception;
  }

}
