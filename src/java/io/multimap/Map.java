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
import io.multimap.Callables.LessThan;
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
   * This class is a pure data holder used for configuring instances of type {@link Map}.
   *
   * @author Martin Trenkmann
   */
  public static class Options {
    /**
     * A static instance useful to obtain the default configuration of the options parameter.
     */
    public static final Options DEFAULT = new Options();

    private int blockSize = 512;
    private int numPartitions = 23;
    private boolean createIfMissing = false;
    private boolean errorIfExists = false;
    private boolean readonly = false;
    private boolean quiet = false;
    private Callables.LessThan lessThan;

    /**
     * Returns the block size in number of bytes.
     *
     * @see #setBlockSize(int)
     */
    public int getBlockSize() {
      return blockSize;
    }

    /**
     * Defines the block size in number of bytes for a newly created map. The value must be a power
     * of two. Typical block sizes are 128, 256, 512 (default), 1024, or even larger. Please refer
     * to
     * the <a href="http://multimap.io/overview#block-organization">overview</a> section on the
     * project's website for more details.
     */
    public void setBlockSize(int blockSize) {
      Check.isPositive(blockSize);
      this.blockSize = blockSize;
    }

    /**
     * Sets the block size to a special value that indicates to the optimize operation that the
     *block
     * size should not be changed.
     *
     * @see Map#optimize(java.nio.file.Path, java.nio.file.Path, Options)
     */
    public void keepBlockSize() {
      blockSize = 0;
    }

    /**
     * Returns the number of partitions.
     *
     * @see #setNumPartitions(int)
     */
    public int getNumPartitions() {
      return numPartitions;
    }

    /**
     * Defines the number of partitions for a newly created map. The purpose of partitioning is to
     * increase the performance of the export and optimize operations by applying a divide and
     * conquer method. A suitable number can be estimated like this: "total number of value-bytes to
     * be put" divided by "the memory allowed to be used running the operation". An underestimate
     *can
     * lead to long runtimes for the mentioned operations. The default value is 23; other values
     *will
     * be rounded to the next prime number that is greater or equal to the given value.
     *
     * @see Map#exportToBase64(java.nio.file.Path, java.nio.file.Path)
     * @see Map#exportToBase64(java.nio.file.Path, java.nio.file.Path, Options)
     * @see Map#optimize(java.nio.file.Path, java.nio.file.Path)
     * @see Map#optimize(java.nio.file.Path, java.nio.file.Path, Options)
     */
    public void setNumPartitions(int numPartitions) {
      Check.isPositive(numPartitions);
      this.numPartitions = numPartitions;
    }

    /**
     * Sets the number of partitions to a special value that indicates to the optimize operation
     *that
     * the number of partitions should not be changed.
     *
     * @see Map#optimize(java.nio.file.Path, java.nio.file.Path, Options)
     */
    public void keepNumPartitions() {
      numPartitions = 0;
    }

    /**
     * Returns {@code true} if a map should be created if it does not already exist, {@code false}
     * otherwise.
     *
     * @see #setCreateIfMissing(boolean)
     */
    public boolean isCreateIfMissing() {
      return createIfMissing;
    }

    /**
     * If set to {@code true}, creates a new map if it does not exist. The default value is
     * {@code false}.
     */
    public void setCreateIfMissing(boolean createIfMissing) {
      this.createIfMissing = createIfMissing;
    }

    /**
     * Returns {@code true} if an error should be raised in case a map already exists, {@code false}
     * otherwise.
     *
     * @see #setErrorIfExists(boolean)
     */
    public boolean isErrorIfExists() {
      return errorIfExists;
    }

    /**
     * If set to {@code true}, throws an exception if a map already exists. The default value is
     * {@code false}.
     */
    public void setErrorIfExists(boolean errorIfExists) {
      this.errorIfExists = errorIfExists;
    }

    /**
     * Returns {@code true} if a map should be opened in read-only mode, {@code false} otherwise.
     *
     * @see #setReadonly(boolean)
     */
    public boolean isReadonly() {
      return readonly;
    }

    /**
     * If set to {@code true}, opens a map in read-only mode. In this mode all operations that could
     * possibly modify the stored data are not allowed and will throw an exception on an attempt to
     * do so. This flag is useful to prevent unintentional updates of read-only datasets. The
     * default
     * value is {@code false}.
     */
    public void setReadonly(boolean readonly) {
      this.readonly = readonly;
    }

    /**
     * Returns {@code true} if no status information for long running operations are sent to stdout,
     * {@code false} otherwise.
     *
     * @see #setQuiet(boolean)
     */
    public boolean isQuiet() {
      return quiet;
    }

    /**
     * If set to {@code true}, no status information for long running operations are sent to stdout.
     * This flag is useful for writing shell scripts. The default value is {@code false}.
     */
    public void setQuiet(boolean quiet) {
      this.quiet = quiet;
    }

    /**
     * Returns the callable used for comparing values. May be {@code null} if no sorting is desired.
     *
     * @see #setLessThanCallable(Callables.LessThan)
     */
    public Callables.LessThan getLessThanCallable() {
      return lessThan;
    }

    /**
     * Sets a callable that implements the {@link LessThan} interface. It is used to sort lists of
     * values when running certain operations such as export or optimize. Can be set to {@code
     *null},
     * which is also the default, if no sorting is desired.
     *
     * @see Map#exportToBase64(java.nio.file.Path, java.nio.file.Path, Options)
     * @see Map#optimize(java.nio.file.Path, java.nio.file.Path, Options)
     */
    public void setLessThanCallable(Callables.LessThan lessThan) {
      this.lessThan = lessThan;
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
      return String.format("block_size        %d\n"
              + "key_size_avg      %d\n"
              + "key_size_max      %d\n"
              + "key_size_min      %d\n"
              + "list_size_avg     %d\n"
              + "list_size_max     %d\n"
              + "list_size_min     %d\n"
              + "num_blocks        %d\n"
              + "num_keys_total    %d\n"
              + "num_keys_valid    %d\n"
              + "num_values_total  %d\n"
              + "num_values_valid  %d\n"
              + "num_partitions    %d\n",
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
   * Appends {@code value} to the end of the list associated with the specified key.
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
   * Returns a read-only iterator for the list associated with the specified key. If the key does
   * not exist, an empty iterator that has no values is returned. A non-empty iterator owns a reader
   * lock on the associated list and must be closed via {@link Iterator#close()} when it
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
   * Returns {@code true} if there is at least one value associated with the specified key,
   * {@code false} otherwise.
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
   * Removes all values from the list associated with the specified key.
   *
   * <p><b>Acquires:</b></p>
   * <ul>
   * <li>a reader lock on the map object.</li>
   * <li>a writer lock on the list associated with {@code key}.</li>
   * </ul>
   *
   * @return the number values that have been removed.
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
   * Removes the first value which is equal to the given one from the list associated with the
   * specified key.
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
  public boolean removeFirstEqual(byte[] key, byte[] value) {
    Check.notNull(key);
    Check.notNull(value);
    return Native.removeFirstEqual(self, key, value);
  }

  /**
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   *
   * @since 0.5.0
   */
  public boolean removeFirstEqual(String key, byte[] value) {
    return removeFirstEqual(Utils.toByteArray(key), value);
  }
  
  /**
   * Removes all values which are equal to the given one from the list associated with the
   * specified key.
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
  public int removeAllEqual(byte[] key, byte[] value) {
    Check.notNull(key);
    Check.notNull(value);
    return Native.removeAllEqual(self, key, value);
  }

  /**
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   *
   * @since 0.5.0
   */
  public int removeAllEqual(String key, byte[] value) {
    return removeAllEqual(Utils.toByteArray(key), value);
  }
  
  /**
   * Removes the first value for which {@code predicate} yields {@code true} from the list
   * associated with the specified key. The predicate can be any callable that implements the
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
  public boolean removeFirstMatch(byte[] key, Predicate predicate) {
    Check.notNull(key);
    Check.notNull(predicate);
    return Native.removeFirstMatch(self, key, predicate);
  }

  /**
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   *
   * @since 0.5.0
   */
  public boolean removeFirstMatch(String key, Predicate predicate) {
    return removeFirstMatch(Utils.toByteArray(key), predicate);
  }
  
  /**
   * Removes all values from the list associated with the first key for which {@code predicate}
   * yields {@code true}. The predicate can be any callable that implements the {@link Predicate}
   * interface.
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
  public int removeFirstMatch(Predicate predicate) {
    return Native.removeFirstMatch(self, predicate);
  }
  
  /**
   * Removes the first value for which {@code predicate} yields {@code true} from the list
   * associated with the specified key. The predicate can be any callable that implements the
   * {@link Predicate} interface.
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
  public int removeAllMatches(byte[] key, Predicate predicate) {
    Check.notNull(key);
    Check.notNull(predicate);
    return Native.removeAllMatches(self, key, predicate);
  }

  /**
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   *
   * @since 0.5.0
   */
  public int removeAllMatches(String key, Predicate predicate) {
    return removeAllMatches(Utils.toByteArray(key), predicate);
  }
  
  /**
   * Removes all values from lists associated with keys for which {@code predicate} yields
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
  public Utils.Pair<Integer, Long> removeAllMatches(Predicate predicate) {
    ByteBuffer buffer = ByteBuffer.wrap(Native.removeAllMatches(self, predicate));
    buffer.order(ByteOrder.LITTLE_ENDIAN);
    return new Utils.Pair<Integer, Long>(buffer.getInt(), buffer.getLong());
  }
  
  /**
   * Replaces the first value which is equal to {@code oldValue} with {@code newValue} in the list
   * associated with the specified key.
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
  public boolean replaceFirstEqual(byte[] key, byte[] oldValue, byte[] newValue) {
    Check.notNull(key);
    Check.notNull(oldValue);
    Check.notNull(newValue);
    return Native.replaceFirstEqual(self, key, oldValue, newValue);
  }

  /**
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   *
   * @since 0.5.0
   */
  public boolean replaceFirstEqual(String key, byte[] oldValue, byte[] newValue) {
    return replaceFirstEqual(Utils.toByteArray(key), oldValue, newValue);
  }
  
  /**
   * Replaces all values which are equal to {@code oldValue} with {@code newValue} in the list
   * associated with the specified key.
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
  public int replaceAllEqual(byte[] key, byte[] oldValue, byte[] newValue) {
    Check.notNull(key);
    Check.notNull(oldValue);
    Check.notNull(newValue);
    return Native.replaceAllEqual(self, key, oldValue, newValue);
  }

  /**
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   *
   * @since 0.5.0
   */
  public int replaceAllEqual(String key, byte[] oldValue, byte[] newValue) {
    return replaceAllEqual(Utils.toByteArray(key), oldValue, newValue);
  }
  
  /**
   * Replaces the first value with the result of invoking {@code map} in the list associated with
   * the specified key. A match is indicated by returning a byte array, not {@code null}, from
   * {@code map} for a given value. The map function can be any callable that implements the
   * {@link Function} interface.
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
  public boolean replaceFirstMatch(byte[] key, Function map) {
    Check.notNull(key);
    Check.notNull(map);
    return Native.replaceFirstMatch(self, key, map);
  }

  /**
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   *
   * @since 0.5.0
   */
  public boolean replaceFirstMatch(String key, Function map) {
    return replaceFirstMatch(Utils.toByteArray(key), map);
  }
  
  /**
   * Replaces all values with the result of invoking {@code map} in the list associated with the
   * specified key. A match is indicated by returning a byte array, not {@code null}, from
   * {@code map} for a given value. The map function can be any callable that implements the
   * {@link Function} interface.
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
  public int replaceAllMatches(byte[] key, Function map) {
    Check.notNull(key);
    Check.notNull(map);
    return Native.replaceAllMatches(self, key, map);
  }

  /**
   * Same as before, but taking {@code key} as string instead of byte array. Internally the key is
   * converted into a byte array via {@link Utils#toByteArray(String)}.
   *
   * @since 0.5.0
   */
  public int replaceAllMatches(String key, Function map) {
    return replaceAllMatches(Utils.toByteArray(key), map);
  }

  /**
   * Applies {@code process} to each key in the map. The process object can be any callable that
   * implements the {@link Procedure} interface.
   *
   * <p><b>Acquires</b> a reader lock on the map object.</p>
   */
  public void forEachKey(Procedure process) {
    Check.notNull(process);
    Native.forEachKey(self, process);
  }

  /**
   * Applies {@code process} to each value in the list associated with the specified key. The
   * process object can be any callable that implements the {@link Procedure} interface.
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
  public static void importFromBase64(Path directory, Path input, Options options)
      throws Exception {
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
    static native boolean removeFirstEqual(ByteBuffer self, byte[] key, byte[] value);
    static native int removeAllEqual(ByteBuffer self, byte[] key, byte[] value);
    static native boolean removeFirstMatch(ByteBuffer self, byte[] key, Predicate predicate);
    static native int removeFirstMatch(ByteBuffer self, Predicate predicate);
    static native int removeAllMatches(ByteBuffer self, byte[] key, Predicate predicate);
    static native byte[] removeAllMatches(ByteBuffer self, Predicate predicate);
    static native boolean replaceFirstEqual(ByteBuffer self, byte[] key, byte[] oldValue, byte[] newValue);
    static native int replaceAllEqual(ByteBuffer self, byte[] key, byte[] oldValue, byte[] newValue);
    static native boolean replaceFirstMatch(ByteBuffer self, byte[] key, Function function);
    static native int replaceAllMatches(ByteBuffer self, byte[] key, Function function);
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
