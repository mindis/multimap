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

import io.multimap.Callables.LessThan;

/**
 * This class is a pure data holder used for configuration purposes.
 * 
 * @author Martin Trenkmann
 */
public class Options {
  
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
   * of two. Typical block sizes are 128, 256, 512 (default), 1024, or even larger. Please refer to
   * the <a href="http://multimap.io/overview#block-organization">overview</a> section on the
   * project's website for more details.
   */
  public void setBlockSize(int blockSize) {
    Check.isPositive(blockSize);
    this.blockSize = blockSize;
  }
  
  /**
   * Sets the block size to a special value that indicates to the optimize operation that the block
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
   * be put" divided by "the memory allowed to be used running the operation". An underestimate can
   * lead to long runtimes for the mentioned operations. The default value is 23; other values will
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
   * Sets the number of partitions to a special value that indicates to the optimize operation that
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
   * do so. This flag is useful to prevent unintentional updates of read-only datasets. The default
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
   * values when running certain operations such as export or optimize. Can be set to {@code null},
   * which is also the default, if no sorting is desired.
   * 
   * @see Map#exportToBase64(java.nio.file.Path, java.nio.file.Path, Options)
   * @see Map#optimize(java.nio.file.Path, java.nio.file.Path, Options)
   */
  public void setLessThanCallable(Callables.LessThan lessThan) {
    this.lessThan = lessThan;
  }

}
