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
   * @see Map#replaceValues(byte[], Callables.Function)
   * @see Map#replaceValue(byte[], Callables.Function)
   */
  public static abstract class Function {

    /**
     * Applies the function to {@code bytes}. This is a wrapper around
     * {@link #callOnReadOnly(ByteBuffer)} which marks the input as read-only.
     * 
     * @return a new {@code byte[]} or {@code null} if explicitly allowed by an implementation.
     * 
     * @see #callOnReadOnly(ByteBuffer)
     */
    public byte[] call(ByteBuffer bytes) {
      return callOnReadOnly(bytes.asReadOnlyBuffer());
    }

    /**
     * Applies the function to {@code bytes}. The input is guaranteed to be read-only. Must be
     * implemented in derived classes.
     * 
     * @return a new {@code byte[]} or {@code null} if explicitly allowed by an implementation.
     */
    protected abstract byte[] callOnReadOnly(ByteBuffer bytes);
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
     * Determines whether {@code a} is less than {@code b}. This is a wrapper around
     * {@link #callOnReadOnly(ByteBuffer, ByteBuffer)} which marks the input as read-only.
     * 
     * @return {@code true} if {@code a} is less than {@code b}, {@code false} otherwise.
     * 
     * @see #callOnReadOnly(ByteBuffer, ByteBuffer)
     */
    public boolean call(ByteBuffer a, ByteBuffer b) {
      return callOnReadOnly(a.asReadOnlyBuffer(), b.asReadOnlyBuffer());
    }

    /**
     * Determines whether {@code a} is less than {@code b}. The input is guaranteed to be read-only.
     * Must be implemented in derived classes.
     * 
     * @return {@code true} if {@code a} is less than {@code b}, {@code false} otherwise.
     */
    protected abstract boolean callOnReadOnly(ByteBuffer a, ByteBuffer b);
  }
  
  /**
   * A callable that is applied to a {@link ByteBuffer} returning a boolean value. Objects
   * implementing this interface are typically used to qualify keys or values for some further
   * operation.
   * 
   * @see Map#removeKeys(Callables.Predicate)
   * @see Map#removeValue(byte[], Callables.Predicate)
   * @see Map#removeValues(byte[], Callables.Predicate)
   */
  public static abstract class Predicate {

    /**
     * Applies the predicate to {@code bytes}. This is a wrapper around
     * {@link #callOnReadOnly(ByteBuffer)} which marks the input as read-only.
     * 
     * @return {@code true} if the predicate matches, {@code false} otherwise.
     * 
     * @see #callOnReadOnly(ByteBuffer)
     */
    public boolean call(ByteBuffer bytes) {
      return callOnReadOnly(bytes.asReadOnlyBuffer());
    }

    /**
     * Applies the predicate to {@code bytes}. The input is guaranteed to be read-only. Must be
     * implemented in derived classes.
     * 
     * @return {@code true} if the predicate matches, {@code false} otherwise.
     */
    protected abstract boolean callOnReadOnly(ByteBuffer bytes);
  }

  /**
   * A callable that is applied to a {@link ByteBuffer} without returning any value. Procedures
   * can have state that may change during application. Objects implementing this interface are
   * typically used to visit keys or values, e.g. to collect information about them.
   * 
   * @see Map#forEachKey(Callables.Procedure)
   * @see Map#forEachValue(byte[], Callables.Procedure)
   */
  public static abstract class Procedure {

    /**
     * Applies the procedure to {@code bytes}. This is a wrapper around
     * {@link #callOnReadOnly(ByteBuffer)} which marks the input as read-only.
     * 
     * @see #callOnReadOnly(ByteBuffer)
     */
    public void call(ByteBuffer bytes) {
      Check.notNull(bytes);
      callOnReadOnly(bytes.asReadOnlyBuffer());
    }

    /**
     * Applies the procedure to {@code bytes}. The input is guaranteed to be read-only. Must be
     * implemented in derived classes.
     */
    protected abstract void callOnReadOnly(ByteBuffer bytes);
  }

}
