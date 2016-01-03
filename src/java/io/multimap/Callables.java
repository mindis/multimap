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
   * Processes a value and returns a boolean. Predicates are used to check a property of a value
   * and, depending on the outcome, control the path of execution. Consider iterating a list of
   * values to take action on only those values for which the predicate yields {@code true}.
   * 
   * @see Map#removeValues(byte[], Predicate)
   * @see Map#removeValue(byte[], Predicate)
   * @see Map#forEachValue(byte[], Predicate)
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
   * Processes a value not returning a result. However, since derived classes may have state, a
   * procedure can be used to collect information about the processed data. Thus, returning a result
   * indirectly. Consider iterating a list of values completely to count certain occurrences.
   * 
   * @see Map#forEachKey(Procedure)
   * @see Map#forEachValue(byte[], Procedure)
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

  /**
   * Processes a value and returns a new one. Functions are used to map an input value to an output
   * value or, if permitted {@code null}. Consider iterating a list of values where all or some of
   * them should be replaced.
   * 
   * @see Map#replaceValues(byte[], Function)
   * @see Map#replaceValue(byte[], Function)
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
   * Processes two values and determines their order. The {@code LessThan} comparator is a predicate
   * used for sorting purposes. Consider sorting a list of values in ascending or descending order.
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

}
