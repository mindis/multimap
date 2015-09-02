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
 * This class represents various options used to parameterize a {@link Map} if opened or created.
 * 
 * @author Martin Trenkmann
 */
public class Options {

  private int blockSize = 512;
  private boolean createIfMissing = false;
  private boolean errorIfExists = false;
  private boolean writeOnlyMode = false;

  /**
   * Tells the block size.
   */
  public int getBlockSize() {
    return blockSize;
  }

  /**
   * Defines the block size for a newly created map. The value must be a power of two and is
   * typically 128, 256, 512, 1024, or even larger. The block size has direct impact on the memory
   * usage and compactness of the {@link Map}, which in turn might affects the overall performance.
   * The default value is {@code 512}. Visit the project's website for more information.
   */
  public void setBlockSize(int numBytes) {
    Check.isPositive(numBytes);
    this.blockSize = numBytes;
  }

  /**
   * Tells whether a {@link Map} should be created if it does not already exist.
   */
  public boolean getCreateIfMissing() {
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
  public boolean getErrorIfExists() {
    return errorIfExists;
  }

  /**
   * Defines whether it is an error if a {@link Map} already exist. This options is only useful if
   * {@link #getCreateIfMissing()} is set to {@code true}. The default value is {@code false}.
   */
  public void setErrorIfExists(boolean errorIfExists) {
    this.errorIfExists = errorIfExists;
  }

  /**
   * Tells whether a {@link Map} should be opened or created in write-only mode.
   */
  public boolean getWriteOnlyMode() {
    return writeOnlyMode;
  }

  /**
   * Defines whether a {@link Map} should be opened or created in write-only mode. Setting this
   * option to {@code true} might lead to better performance when only
   * {@link Map#put(byte[], byte[])} is required. The default value is {@code false}.
   */
  public void setWriteOnlyMode(boolean writeOnlyMode) {
    this.writeOnlyMode = writeOnlyMode;
  }

  @Override
  public String toString() {
    char delim = '=';
    char newline = '\n';
    StringBuilder sb = new StringBuilder();
    sb.append("block-size").append(delim).append(getBlockSize()).append(newline);
    sb.append("create-if-missing").append(delim).append(getCreateIfMissing()).append(newline);
    sb.append("error-if-exists").append(delim).append(getErrorIfExists()).append(newline);
    sb.append("write-only-mode").append(delim).append(getWriteOnlyMode()).append(newline);
    return sb.toString();
  }

}
