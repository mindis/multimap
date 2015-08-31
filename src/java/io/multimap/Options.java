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

public class Options {

  private int blockSize = 512;
  private boolean createIfMissing = false;
  private boolean errorIfExists = false;
  private boolean writeOnlyMode = false;
  private long writeBufferSize = MiB(10);

  public int getBlockSize() {
    return blockSize;
  }

  public void setBlockSize(int numBytes) {
    Check.isPositive(numBytes);
    this.blockSize = numBytes;
  }

  public boolean getCreateIfMissing() {
    return createIfMissing;
  }

  public void setCreateIfMissing(boolean createIfMissing) {
    this.createIfMissing = createIfMissing;
  }

  public boolean getErrorIfExists() {
    return errorIfExists;
  }

  public void setErrorIfExists(boolean errorIfExists) {
    this.errorIfExists = errorIfExists;
  }

  public boolean getWriteOnlyMode() {
    return writeOnlyMode;
  }

  public void setWriteOnlyMode(boolean writeOnlyMode) {
    this.writeOnlyMode = writeOnlyMode;
  }

  public long getWriteBufferSize() {
    return writeBufferSize;
  }

  public void setWriteBufferSize(long numBytes) {
    Check.isPositive(numBytes);
    this.writeBufferSize = numBytes;
  }

  @Override
  public String toString() {
    char delim = '=';
    char newline = '\n';
    StringBuilder sb = new StringBuilder();
    sb.append("block-size").append(delim).append(getBlockSize()).append(newline);
    sb.append("create-if-missing").append(delim).append(getCreateIfMissing()).append(newline);
    sb.append("error-if-exists").append(delim).append(getErrorIfExists()).append(newline);
    sb.append("write-only-mode").append(delim).append(getWriteBufferSize()).append(newline);
    sb.append("write-buffer-size").append(delim).append(getWriteBufferSize()).append(newline);
    return sb.toString();
  }

  public static final long MiB(int numMebibytes) {
    return (long) numMebibytes << 20;
  }

  public static final long GiB(int numGibibytes) {
    return (long) numGibibytes << 30;
  }

}
