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

import java.nio.ByteBuffer;

/**
 * This class provides abstract base classes that represent various callable types.
 * 
 * @author Martin Trenkmann
 */
public final class Callables {

  private Callables() {}

  /**
   * A callable that is applied to a {@link ByteBuffer} returning a newly allocated {@code byte[]}.
   * Objects implementing this interface are typically used for mapping input values to output
   * values in replace operations.
   * 
   * @see Map#replaceFirstMatch(byte[], Callables.Function)
   * @see Map#replaceAllMatches(byte[], Callables.Function)
   */
  public static abstract class Function {

    /**
     * Applies the function to {@code bytes}. The given byte buffer should be treated as read-only,
     * even if {@link ByteBuffer#isReadOnly()} yields false. Attempts to write into the given buffer
     * might let the VM crash.
     * 
     * @return a new {@code byte[]} or {@code null} if explicitly allowed by an implementation.
     */
    public abstract byte[] call(ByteBuffer bytes);
  }
  
  /**
   * A callable that is applied to two {@link ByteBuffer}'s returning a boolean that tells if the
   * left operand is less than the right operand. Objects implementing this interface are typically
   * used by sorting algorithms.
   * 
   * @see Map#optimize(java.nio.file.Path, java.nio.file.Path, Options)
   */
  public static abstract class LessThan {

    /**
     * Determines whether {@code a} is less than {@code b}. The given byte buffers should be treated
     * as read-only, even if {@link ByteBuffer#isReadOnly()} yields false. Attempts to write into
     * the given buffers might let the VM crash.
     * 
     * @return {@code true} if {@code a} is less than {@code b}, {@code false} otherwise.
     */
    public abstract boolean call(ByteBuffer a, ByteBuffer b);
  }
  
  /**
   * A callable that is applied to a {@link ByteBuffer} returning a boolean value. Objects
   * implementing this interface are typically used to qualify keys or values for some further
   * operation.
   * 
   * @see Map#removeFirstMatch(byte[], Callables.Predicate)
   * @see Map#removeAllMatches(byte[], Callables.Predicate)
   */
  public static abstract class Predicate {

    /**
     * Applies the predicate to {@code bytes}. The given byte buffer should be treated as read-only,
     * even if {@link ByteBuffer#isReadOnly()} yields false. Attempts to write into the given buffer
     * might let the VM crash.
     * 
     * @return {@code true} if the predicate matches, {@code false} otherwise.
     */
    public abstract boolean call(ByteBuffer bytes);
  }

  /**
   * A callable that is applied to a {@link ByteBuffer} without returning any value. Procedures
   * often maintain an internal state that changes during application. Objects implementing this
   * interface are typically used to visit keys or values, e.g. to collect information about them.
   * 
   * @see Map#forEachKey(Callables.Procedure)
   * @see Map#forEachValue(byte[], Callables.Procedure)
   */
  public static abstract class Procedure {

    /**
     * Applies the procedure to {@code bytes}. The given byte buffer should be treated as read-only,
     * even if {@link ByteBuffer#isReadOnly()} yields false. Attempts to write into the given buffer
     * might let the VM crash.
     */
    public abstract void call(ByteBuffer bytes);
  }

}
