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

/**
 * This class is a pure data holder used to configure an instantiation of class {@link Map}.
 * 
 * @author Martin Trenkmann
 */
public class Options {
  
  private int blockSize = 512;
  private int numPartitions = 23;
  private boolean createIfMissing = false;
  private boolean errorIfExists = false;
  private boolean readonly = false;
  private boolean quiet = false;
  private Callables.LessThan lessThan;

  /**
   * Tells the block size.
   */
  public int getBlockSize() {
    return blockSize;
  }

  /**
   * Defines the block size for a newly created map. The value must be a power of two and is
   * typically 128, 256, 512, 1024, or even larger. The block size has direct impact on the memory
   * usage of the {@link Map}, which in turn might affects the overall performance. The default
   * value is {@code 512}. Visit the project's website for more information.
   */
  public void setBlockSize(int blockSize) {
    Check.isPositive(blockSize);
    this.blockSize = blockSize;
  }
  
  public void keepBlockSize() {
    blockSize = 0;
  }
  
  public int getNumPartitions() {
    return numPartitions;
  }

  public void setNumPartitions(int numShards) {
    Check.isPositive(numShards);
    this.numPartitions = numShards;
  }
  
  public void keepNumPartitions() {
    numPartitions = 0;
  }
  
  /**
   * Tells whether a {@link Map} should be created if it does not already exist.
   */
  public boolean isCreateIfMissing() {
    return createIfMissing;
  }

  /**
   * Defines whether a {@link Map} should be created if it does not already exist. The default value
   * is {@code false}.
   */
  public void setCreateIfMissing(boolean createIfMissing) {
    this.createIfMissing = createIfMissing;
  }

  /**
   * Tells whether it is an error if a {@link Map} already exist.
   */
  public boolean isErrorIfExists() {
    return errorIfExists;
  }

  /**
   * Defines whether it is an error if a {@link Map} already exist. This options is only useful if
   * {@link #getCreateIfMissing()} is set to {@code true}. The default value is {@code false}.
   */
  public void setErrorIfExists(boolean errorIfExists) {
    this.errorIfExists = errorIfExists;
  }
  
  public boolean isReadonly() {
    return readonly;
  }

  public void setReadonly(boolean readonly) {
    this.readonly = readonly;
  }

  public boolean isQuiet() {
    return quiet;
  }

  public void setQuiet(boolean quiet) {
    this.quiet = quiet;
  }

  public Callables.LessThan getLessThanCallable() {
    return lessThan;
  }

  public void setLessThanCallable(Callables.LessThan lessThan) {
    this.lessThan = lessThan;
  }

}
