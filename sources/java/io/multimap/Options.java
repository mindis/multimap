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
  private long blockPoolMemory = GiB(1);
  private boolean createIfMissing = false;
  private boolean verbose = false;
  
  public int getBlockSize() {
    return blockSize;
  }
  
  public void setBlockSize(int numBytes) {
    Check.isPositive(numBytes);
    this.blockSize = numBytes;
  }
  
  public long getBlockPoolMemory() {
    return blockPoolMemory;
  }
  
  public void setBlockPoolMemory(long numBytes) {
    Check.isPositive(numBytes);
    this.blockPoolMemory = numBytes;
  }
  
  public boolean canCreateIfMissing() {
    return createIfMissing;
  }
  
  public void setCreateIfMissing(boolean createIfMissing) {
    this.createIfMissing = createIfMissing;
  }
  
  public boolean isVerbose() {
    return verbose;
  }
  
  public void setVerbose(boolean verbose) {
    this.verbose = verbose;
  }
  
  @Override
  public String toString() {
    char delim = '=';
    char newline = '\n';
    StringBuilder sb = new StringBuilder();
    sb.append("block-size").append(delim).append(getBlockSize()).append(newline);
    sb.append("block-pool-memory").append(delim).append(getBlockPoolMemory()).append(newline);
    sb.append("create-if-missing").append(delim).append(canCreateIfMissing()).append(newline);
    sb.append("verbose").append(delim).append(isVerbose());
    return sb.toString();
  }
  
  public static final long MiB(int numMebibytes) {
    return (long) numMebibytes << 20;
  }
  
  public static final long GiB(int numGibibytes) {
    return (long) numGibibytes << 30;
  }
  
}
