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

import io.multimap.Callables.Predicate;

import java.nio.ByteBuffer;

/**
 * This abstract class represents a forward iterator on a list of values. In contrast to the
 * standard {@link Iterator} interface this class has the following properties:
 * 
 * <ul>
 * <li>Support for lazy initialization. An iterator does not perform any IO operation until
 * {@link Iterator#seekToFirst()}, {@link Iterator#seekTo(byte[])}, or
 * {@link Iterator#seekTo(Predicate)} has been called. This might be useful in cases where multiple
 * iterators have to be requested first to determine in which order they have to be processed.</li>
 * <li>The iterator can state the total number of the underlying values, even if
 * {@link Iterator#seekToFirst()} or one of its friends have not been called.</li>
 * <li>The iterator does not advance automatically. The current value can be retrieved multiple
 * times.</li>
 * <li>The iterator must be closed if no longer needed to release native resources such as locks.
 * Not closing an iterator that implements a locking mechanism leaves the underlying list in locked
 * state which leads to deadlocks sooner or later.</li>
 * </ul>
 * 
 * @author Martin Trenkmann
 */
public abstract class Iterator implements AutoCloseable {

  /**
   * Returns the total number of values to iterate. This number does not change when the iterator
   * moves forward. The method may be called at any time, even if {@link #seekToFirst()} or one of
   * its friends have not been called.
   */
  public abstract long numValues();

  /**
   * Initializes the iterator to point to the first value, if any. This process will trigger disk
   * IO if necessary. The method can also be used to seek back to the beginning of the list at the
   * end of an iteration.
   */
  public abstract void seekToFirst();

  /**
   * Initializes the iterator to point to the first value in the list that is equal to
   * {@code target}, if any. This process will trigger disk IO if necessary.
   */
  public abstract void seekTo(byte[] target);

  /**
   * Initializes the iterator to point to the first value for which {@code predicate} yields
   * {@code true}, if any. This process will trigger disk IO if necessary.
   */
  public abstract void seekTo(Predicate predicate);

  /**
   * Tells whether the iterator points to a value. If the result is {@code true}, the iterator may
   * be dereferenced via {@link #getValue()} or {@link #getValueAsByteArray()}.
   */
  public abstract boolean hasValue();

  /**
   * Returns the current value as {@link ByteBuffer}. The returned buffer is a direct one, i.e. it
   * wraps a pointer that points directly into native allocated memory not managed by the Java
   * Garbage Collector. This pointer is only valid as long as the iterator does not move forward.
   * Therefore, the buffer should only be used to immediately parse information or some user-defined
   * object out of it. Most serialization libraries should be able to consume a byte buffer. If a
   * byte array is required or an independent copy of the value use {@link #getValueAsByteArray()}
   * instead.
   */
  public abstract ByteBuffer getValue();

  /**
   * Returns the current value as byte array. The returned array contains a copy of the value's
   * bytes and is managed by the Java Garbage Collector, in contrast to the outcome of
   * {@link #getValue()}. It can be copied around or stored in a collection as any other Java
   * object.
   */
  public byte[] getValueAsByteArray() {
    ByteBuffer value = getValue();
    byte[] copy = new byte[value.capacity()];
    value.get(copy);
    return copy;
  }

  /**
   * Deletes the value the iterator currently points to (optional operation).
   */
  public abstract void deleteValue();

  /**
   * Moves the iterator to the next value, if any.
   */
  public abstract void next();

  /**
   * A constant that represents an empty iterator.
   */
  public static final Iterator EMPTY = new Iterator() {

    @Override
    public long numValues() {
      return 0;
    }

    @Override
    public void seekToFirst() {}

    @Override
    public void seekTo(byte[] target) {}

    @Override
    public void seekTo(Predicate predicate) {}

    @Override
    public boolean hasValue() {
      return false;
    }

    @Override
    public ByteBuffer getValue() {
      return null;
    }

    @Override
    public void deleteValue() {}

    @Override
    public void next() {}

    @Override
    public void close() {}

  };

}