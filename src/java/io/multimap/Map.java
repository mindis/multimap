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
import java.nio.ByteOrder;
import java.nio.file.Path;
import java.nio.file.Paths;

/**
 * This class implements a 1:n key-value store where each key is associated with a list of values.
 * 
 * <br>
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

  /**
   * This type provides static methods for obtaining system limitations. Those limits which define
   * constraints on user supplied data also serve as preconditions.
   * 
   * @author Martin Trenkmann
   * @since 0.3.0
   */
  public static class Limits {

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
  
  /**
   * This type is a pure data holder for reporting statistical information.
   * 
   * @author Martin Trenkmann
   * @since 0.4.0
   */
  public static class Stats {
    
    private Stats() {}
    
    /**
     * Returns the block size of the map or partition.
     */
    public long getBlockSize() {
      return blockSize;
    }
    
    /**
     * Returns the average size in number of bytes of all keys in a map or partition. Note that
     * keys that are currently not associated with any value are not taken into account.
     */
    public long getKeySizeAvg() {
      return keySizeAvg;
    }
    
    /**
     * Returns the size in number of bytes of the largest key in a map or partition.
     */
    public long getKeySizeMax() {
      return keySizeMax;
    }
    
    /**
     * Returns the size in number of bytes of the smallest key in a map or partition.
     */
    public long getKeySizeMin() {
      return keySizeMin;
    }
    
    /**
     * Returns the average number of values associated with a key in a map or partition. Note that
     * keys that are currently not associated with any value are not taken into account.
     */
    public long getListSizeAvg() {
      return listSizeAvg;
    }
    
    /**
     * Returns the largest number of values associated with a key in a map or partition.
     */
    public long getListSizeMax() {
      return listSizeMax;
    }
    
    /**
     * Returns the smallest number of values associated with a key in a map or partition. Note that
     * keys that are currently not associated with any value are not taken into account.
     */
    public long getListSizeMin() {
      return listSizeMin;
    }
    
    /**
     * Returns the number of blocks currently written to disk. Note that in-memory write buffer
     * blocks that are associated with each keys are not taken into account. For more details
     * please refer to the <a href="http://multimap.io/overview">overview</a> section of the
     * project's website.
     */
    public long getNumBlocks() {
      return numBlocks;
    }
    
    /**
     * Returns the total number of keys in a map or partition, including keys that are currently
     * not associated with any value.
     */
    public long getNumKeysTotal() {
      return numKeysTotal;
    }
    
    /**
     * Returns the number of valid keys in a map or partition. A valid key is associated with at
     * least one value.
     */
    public long getNumKeysValid() {
      return numKeysValid;
    }
    
    /**
     * Returns the total number of values in a map or partition, including values that are marked
     * as removed. Note that this number can only be decreased by running an optimize operation.
     */
    public long getNumValuesTotal() {
      return numValuesTotal;
    }
    
    /**
     * Returns the number of values in a map or partition that are not marked as removed.
     */
    public long getNumValuesValid() {
      return numValuesValid;
    }
    
    /**
     * Returns the number of partitions in a map. The value could be different from that specified
     * in options when creating a map due to the fact that the next prime number has been chosen.
     * For partition-specific statistics the value is set to 0.
     */
    public long getNumPartitions() {
      return numPartitions;
    }

    @Override
    public String toString() {
      return String.format(
          "block_size        %d\n" +
          "key_size_avg      %d\n" +
          "key_size_max      %d\n" +
          "key_size_min      %d\n" +
          "list_size_avg     %d\n" +
          "list_size_max     %d\n" +
          "list_size_min     %d\n" +
          "num_blocks        %d\n" +
          "num_keys_total    %d\n" +
          "num_keys_valid    %d\n" +
          "num_values_total  %d\n" +
          "num_values_valid  %d\n" +
          "num_partitions    %d\n",
          blockSize, keySizeAvg, keySizeMax, keySizeMin, listSizeAvg, listSizeMax, listSizeMin,
          numBlocks, numKeysTotal, numKeysValid, numValuesTotal, numValuesValid, numPartitions);
    }

    protected void parseFromBuffer(ByteBuffer buffer) {
      buffer.order(ByteOrder.LITTLE_ENDIAN);
      blockSize = buffer.getLong();
      keySizeAvg = buffer.getLong();
      keySizeMax = buffer.getLong();
      keySizeMin = buffer.getLong();
      listSizeAvg = buffer.getLong();
      listSizeMax = buffer.getLong();
      listSizeMin = buffer.getLong();
      numBlocks = buffer.getLong();
      numKeysTotal = buffer.getLong();
      numKeysValid = buffer.getLong();
      numValuesTotal = buffer.getLong();
      numValuesValid = buffer.getLong();
      numPartitions = buffer.getLong();
    }

    // Needs to be synchronized with struct Table::Stats in C++.
    private long blockSize;
    private long keySizeAvg;
    private long keySizeMax;
    private long keySizeMin;
    private long listSizeAvg;
    private long listSizeMax;
    private long listSizeMin;
    private long numBlocks;
    private long numKeysTotal;
    private long numKeysValid;
    private long numValuesTotal;
    private long numValuesValid;
    private long numPartitions;
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
  public Map(Path directory) throws Exception {
    this(directory, new Options());
  }
  
  /**
   * Same as before, but taking {@code directory} as string.
   */
  public Map(String directory) throws Exception {
    this(Paths.get(directory), new Options());
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
  public Map(Path directory, Options options) throws Exception {
    this(Native.newMap(directory.toString(), options));
  }
  
  /**
   * Same as before, but taking {@code directory} as string.
   */
  public Map(String directory, Options options) throws Exception {
    this(Paths.get(directory), options);
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
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   * 
   * @since 0.4.0
   */
  public void put(String key, byte[] value) throws Exception {
    Native.put(self, Utils.toByteArray(key), value);
  }
  
  /**
   * Same as before, but taking both {@code key} and {@code value} as string instead of byte array.
   * Internally the key and the value are converted into byte arrays via
   * {@link Utils#toByteArray(String)}.
   * 
   * @since 0.4.0
   */
  public void put(String key, String value) throws Exception {
    Native.put(self, Utils.toByteArray(key), Utils.toByteArray(value));
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
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   * 
   * @since 0.4.0
   */
  public Iterator get(String key) {
    return get(Utils.toByteArray(key));
  }
  
  /**
   * Returns {@code true} if {@code key} is associated with at least one value, {@code false}
   * otherwise.
   */
  public boolean contains(byte[] key) {
    Check.notNull(key);
    return Native.contains(self, key);
  }
  
  /**
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   * 
   * @since 0.4.0
   */
  public boolean contains(String key) {
    return contains(Utils.toByteArray(key));
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
   * @return the number values removed.
   * @since 0.5.0
   */
  public int remove(byte[] key) {
    Check.notNull(key);
    return Native.remove(self, key);
  }
  
  /**
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   * 
   * @since 0.5.0
   */
  public int remove(String key) {
    return remove(Utils.toByteArray(key));
  }

  /**
   * Removes the <i>first</i> key (and all its associated values) for which {@code predicate}
   * yields {@code true}. The predicate can be any callable that implements the {@link Predicate}
   * interface. Note that since the order of keys is undefined, this method should only be used if
   * either one or no key at all will match.
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on lists associated with matching keys.</li>
   * </ul>
   * 
   * @return the number of values that have been removed.
   * @since 0.5.0
   */
  public int removeOne(Predicate predicate) {
    return Native.removeOne(self, predicate);
  }
  
  /**
   * Removes <i>all</i> keys (and all its associated values) for which {@code predicate} yields
   * {@code true}. The predicate can be any callable that implements the {@link Predicate}
   * interface.
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on lists associated with matching keys.</li>
   * </ul>
   * 
   * @return a pair whose first element tells the number of keys and the second element the total
   *         number of values that have been removed.
   * @since 0.5.0
   */
  public Utils.Pair<Integer, Long> removeAll(Predicate predicate) {
    ByteBuffer result = Native.removeAll(self, predicate);
    return new Utils.Pair<Integer, Long>(result.getInt(), result.getLong());
  }

  /**
   * Removes the <i>first</i> value from the list associated with {@code key} that is equal to
   * {@code value} after byte-wise comparison.
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on the list associated with {@code key}.</li>
   * </ul>
   * 
   * @return {@code true} if any value has been removed, {@code false} otherwise.
   * @since 0.5.0
   */
  public boolean removeOne(byte[] key, byte[] value) {
    Check.notNull(key);
    Check.notNull(value);
    return Native.removeOne(self, key, value);
  }
  
  /**
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   * 
   * @since 0.5.0
   */
  public boolean removeOne(String key, byte[] value) {
    return removeOne(Utils.toByteArray(key), value);
  }
  
  /**
   * Same as before, but taking both {@code key} and {@code value} as string instead of byte array.
   * Internally the key and the value are converted into byte arrays via
   * {@link Utils#toByteArray(String)}.
   * 
   * @since 0.5.0
   */
  public boolean removeOne(String key, String value) {
    return removeOne(Utils.toByteArray(key), Utils.toByteArray(value));
  }

  /**
   * Removes the <i>first</i> value from the list associated with {@code key} for which
   * {@code predicate} yields {@code true}. The predicate can be any callable that implements the
   * {@link Predicate} interface.
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on the list associated with {@code key}.</li>
   * </ul>
   * 
   * @return {@code true} if any value has been removed, {@code false} otherwise.
   * @since 0.5.0
   */
  public boolean removeOne(byte[] key, Predicate predicate) {
    Check.notNull(key);
    Check.notNull(predicate);
    return Native.removeOne(self, key, predicate);
  }
  
  /**
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   * 
   * @since 0.5.0
   */
  public boolean removeOne(String key, Predicate predicate) {
    return removeOne(Utils.toByteArray(key), predicate);
  }

  /**
   * Removes <i>all</i> values from the list associated with {@code key} that are equal to
   * {@code value} after byte-wise comparison.
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on the list associated with {@code key}.</li>
   * </ul>
   * 
   * @return the number of values that have been removed.
   * @since 0.5.0
   */
  public int removeAll(byte[] key, byte[] value) {
    Check.notNull(key);
    Check.notNull(value);
    return Native.removeAll(self, key, value);
  }
  
  /**
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   * 
   * @since 0.5.0
   */
  public int removeAll(String key, byte[] value) {
    return removeAll(Utils.toByteArray(key), value);
  }
  
  /**
   * Same as before, but taking both {@code key} and {@code value} as string instead of byte array.
   * Internally the key and the value are converted into byte arrays via
   * {@link Utils#toByteArray(String)}.
   * 
   * @since 0.5.0
   */
  public int removeAll(String key, String value) {
    return removeAll(Utils.toByteArray(key), Utils.toByteArray(value));
  }

  /**
   * Removes <i>all</i> values from the list associated with {@code key} for which {@code predicate}
   * yields {@code true}. The predicate can be any callable that implements the {@link Predicate}
   * interface.
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on the list associated with {@code key}.</li>
   * </ul>
   * 
   * @return the number of values that have been removed.
   * @since 0.5.0
   */
  public int removeAll(byte[] key, Predicate predicate) {
    Check.notNull(key);
    Check.notNull(predicate);
    return Native.removeAll(self, key, predicate);
  }
  
  /**
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   * 
   * @since 0.5.0
   */
  public int removeAll(String key, Predicate predicate) {
    return removeAll(Utils.toByteArray(key), predicate);
  }

  /**
   * Replaces the <i>first</i> value in the list associated with {@code key} that is equal to
   * {@code oldValue} by {@code newValue}.
   *
   * <p>Note that a replace operation is actually implemented in terms of a remove of the old value
   * followed by an insert/put of the new value. Hence, the new value is always the last value in
   * the list. In other words, the replacement is not in-place.</p>
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on the list associated with {@code key}.</li>
   * </ul>
   * 
   * @return {@code true} if any value has been replaced, {@code false} otherwise.
   * @since 0.5.0
   */
  public boolean replaceOne(byte[] key, byte[] oldValue, byte[] newValue) {
    Check.notNull(key);
    Check.notNull(oldValue);
    Check.notNull(newValue);
    return Native.replaceOne(self, key, oldValue, newValue);
  }
  
  /**
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   * 
   * @since 0.5.0
   */
  public boolean replaceOne(String key, byte[] oldValue, byte[] newValue) {
    return replaceOne(Utils.toByteArray(key), oldValue, newValue);
  }
  
  /**
   * Same as before, but taking all arguments as strings instead of byte arrays. Internally the
   * arguments are converted into byte arrays via {@link Utils#toByteArray(String)}.
   * 
   * @since 0.5.0
   */
  public boolean replaceOne(String key, String oldValue, String newValue) {
    return replaceOne(Utils.toByteArray(key), Utils.toByteArray(oldValue),
        Utils.toByteArray(newValue));
  }

  /**
   * Replaces the <i>first</i> value in the list associated with {@code key} by the result of
   * invoking {@code map}. Values for which {@code map} returns {@code null} are not replaced. The
   * map function can be any callable that implements the {@link Function} interface.
   * 
   * <p>Note that a replace operation is actually implemented in terms of a remove of the old value
   * followed by an insert/put of the new value. Hence, the new value is always the last value in
   * the list. In other words, the replacement is not in-place.</p>
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on the list associated with {@code key}.</li>
   * </ul>
   * 
   * @return {@code true} if any value has been replaced, {@code false} otherwise.
   * @since 0.5.0
   */
  public boolean replaceOne(byte[] key, Function map) {
    Check.notNull(key);
    Check.notNull(map);
    return Native.replaceOne(self, key, map);
  }
  
  /**
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   * 
   * @since 0.5.0
   */
  public boolean replaceOne(String key, Function map) {
    return replaceOne(Utils.toByteArray(key), map);
  }

  /**
   * Replaces <i>all</i> values in the list associated with {@code key} that are equal to
   * {@code oldValue} by {@code newValue}.
   * 
   * <p>Note that a replace operation is actually implemented in terms of a remove of the old value
   * followed by an insert/put of the new value. Hence, the new value is always the last value in
   * the list. In other words, the replacement is not in-place.</p>
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on the list associated with {@code key}.</li>
   * </ul>
   * 
   * @return the number of values that have been replaced.
   * @since 0.5.0
   */
  public int replaceAll(byte[] key, byte[] oldValue, byte[] newValue) {
    Check.notNull(key);
    Check.notNull(oldValue);
    Check.notNull(newValue);
    return Native.replaceAll(self, key, oldValue, newValue);
  }
  
  /**
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   * 
   * @since 0.5.0
   */
  public int replaceAll(String key, byte[] oldValue, byte[] newValue) {
    return replaceAll(Utils.toByteArray(key), oldValue, newValue);
  }
  
  /**
   * Same as before, but taking all arguments as strings instead of byte arrays. Internally the
   * arguments are converted into byte arrays via {@link Utils#toByteArray(String)}.
   * 
   * @since 0.5.0
   */
  public int replaceAll(String key, String oldValue, String newValue) {
    return replaceAll(Utils.toByteArray(key), Utils.toByteArray(oldValue),
        Utils.toByteArray(newValue));
  }
  
  /**
   * Replaces <i>all</i> values in the list associated with {@code key} by the result of invoking
   * {@code map}. Values for which {@code map} returns {@code null} are not replaced. The map
   * function can be any callable that implements the {@link Function} interface.
   * 
   * <p>Note that a replace operation is actually implemented in terms of a remove of the old value
   * followed by an insert/put of the new value. Hence, the new value is always the last value in
   * the list. In other words, the replacement is not in-place.</p>
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on the list associated with {@code key}.</li>
   * </ul>
   * 
   * @return the number of values that have been replaced.
   * @since 0.5.0
   */
  public int replaceAll(byte[] key, Function map) {
    Check.notNull(key);
    Check.notNull(map);
    return Native.replaceAll(self, key, map);
  }
  
  /**
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   * 
   * @since 0.5.0
   */
  public int replaceAll(String key, Function map) {
    return replaceAll(Utils.toByteArray(key), map);
  }

  /**
   * Applies {@code process} to each key in the map. The process argument can be any callable that
   * implements the {@link Procedure} interface. 
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
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   * 
   * @since 0.4.0
   */
  public void forEachValue(String key, Procedure process) {
    forEachValue(Utils.toByteArray(key), process);
  }

  /*
   * The method `forEachEntry` as provided in the C++ version of this library is currently not
   * supported. Consider submitting a feature request if you really need it.
   */

  /**
   * Returns statistical information about the map. Be aware that this operation requires to visit
   * each key-list entry of the map. Extensive use will slow down your application.
   * 
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a reader lock on the list that is currently visited.</li>
   * </ul>
   * 
   * @since 0.4.0
   */
  public Stats getStats() {
    Stats stats = new Stats();
    Native.getStats(self, stats);
    return stats;
  }

  /**
   * Returns {@code true} if the map is read-only, {@code false} otherwise.
   */
  public boolean isReadOnly() {
    return Native.isReadOnly(self);
  }

  /**
   * Flushes all data to disk and ensures that the map is stored in consistent state.
   * This method must be called when an application shuts down in order to avoid data loss.
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

  /**
   * Returns statistical information about each partition of the map located in directory. This
   * method is similar to {@link #getStats()} except that the map does not need to be instantiated.
   * 
   * <p><b>Acquires</b> a directory lock on directory.</p>
   * 
   * @throws Exception if one of the following is true:
   * <ul>
   * <li>the directory does not exist</li>
   * <li>the directory cannot be locked</li>
   * <li>the directory does not contain a map</li>
   * </ul>
   * 
   * @since 0.4.0
   */
  public static Stats stats(Path directory) throws Exception {
    Check.notNull(directory);
    Stats stats = new Stats();
    Native.stats(directory.toString(), stats);
    return stats;
  }

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
   * Same as before, but gives the user more control by providing an {@link Options} parameter
   * which is passed to the constructor of Map when opening. This way a map can be created if it
   * does not already exist.
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
   * Same as before, but gives the user more control by providing an {@link Options} parameter.
   * Most users will use this to pass a compare function that triggers a sorting of all lists
   * before exporting them.
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
   * Same as before, but gives the user more control by providing an {@link Options} parameter.
   * Most users will use this to pass a compare function that triggers a sorting of all lists.
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
    static native boolean contains(ByteBuffer self, byte[] key);
    static native int remove(ByteBuffer self, byte[] key);
    static native int removeOne(ByteBuffer self, Predicate predicate);
    static native ByteBuffer removeAll(ByteBuffer self, Predicate predicate);
    static native boolean removeOne(ByteBuffer self, byte[] key, byte[] value);
    static native boolean removeOne(ByteBuffer self, byte[] key, Predicate predicate);
    static native int removeAll(ByteBuffer self, byte[] key, byte[] value);
    static native int removeAll(ByteBuffer self, byte[] key, Predicate predicate);
    static native boolean replaceOne(ByteBuffer self, byte[] key, byte[] oldValue, byte[] newValue);
    static native boolean replaceOne(ByteBuffer self, byte[] key, Function function);
    static native int replaceAll(ByteBuffer self, byte[] key, byte[] oldValue, byte[] newValue);
    static native int replaceAll(ByteBuffer self, byte[] key, Function function);
    static native void forEachKey(ByteBuffer self, Procedure process);
    static native void forEachValue(ByteBuffer self, byte[] key, Procedure process);
    static native void getStats(ByteBuffer self, Stats stats);
    static native boolean isReadOnly(ByteBuffer self);
    static native void close(ByteBuffer self);
    static native void stats(String directory, Stats stats) throws Exception;
    static native void importFromBase64(String directory, String input, Options options) throws Exception;
    static native void exportToBase64(String directory, String output, Options options) throws Exception;
    static native void optimize(String directory, String output, Options options) throws Exception;
  }
}
