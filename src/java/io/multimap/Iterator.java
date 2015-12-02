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

import java.nio.ByteBuffer;

/**
 * This abstract class represents a forward iterator on a list of values. In contrast to the
 * standard {@link java.util.Iterator} interface this class has the following properties:
 * 
 * <ul>
 * <li>Lazy initialization. An iterator does not perform any IO operation until {@link #next()} has
 * been called. This might be useful in cases where multiple iterators have to be requested first
 * to determine in which order they have to be processed.</li>
 * <li>The iterator can tell the number of remaining values via {@link #available()}.</li>
 * <li>The iterator can emit the next value without moving forward via {@link #peekNext()}.</li>
 * <li>The iterator must be closed if no longer needed to release native resources such as locks.
 * Not closing an iterator that implements a locking mechanism leaves the underlying list in locked
 * state which leads to deadlocks sooner or later.</li>
 * </ul>
 * 
 * @author Martin Trenkmann
 */
public abstract class Iterator implements AutoCloseable {
  
  /**
   * Returns the number of values left to iterate.
   */
  public abstract long available();

  /**
   * Tells whether the iterator has one more value.
   */
  public abstract boolean hasNext();

  /**
   * Returns the next value as {@link ByteBuffer}. The returned buffer is a direct one, i.e. it
   * wraps a pointer that points directly into native allocated memory not managed by the Java
   * Garbage Collector. This pointer is only valid as long as the iterator does not move forward.
   * Therefore, the buffer should only be used to immediately parse the original value out of it.
   * Most serialization libraries should be able to consume this byte buffer. If a byte array is
   * required or an independent copy of the value use {@link #nextAsByteArray()} instead.
   */
  public abstract ByteBuffer next();

  public abstract ByteBuffer peekNext();

  /**
   * Returns the next value as byte array. The returned array contains a copy of the value's bytes
   * and is managed by the Java Garbage Collector, in contrast to the outcome of {@link #next()}. It
   * can be copied around or stored in a collection as any other Java object.
   */
  public byte[] nextAsByteArray() {
    ByteBuffer value = next();
    byte[] copy = new byte[value.capacity()];
    value.get(copy);
    return copy;
  }

  public byte[] peekNextAsByteArray() {
    ByteBuffer value = peekNext();
    byte[] copy = new byte[value.capacity()];
    value.get(copy);
    return copy;
  }

  /**
   * Removes the last value returned by this iterator (optional operation). This method can be
   * called only once per call to {@link #next()} or {@link #nextAsByteArray()}.
   */
  public abstract void remove();

  /**
   * A constant that represents an empty iterator.
   */
  public static final Iterator EMPTY = new Iterator() {

    @Override
    public long available() {
      return 0;
    }

    @Override
    public boolean hasNext() {
      return false;
    }

    @Override
    public ByteBuffer next() {
      return null;
    }

    @Override
    public ByteBuffer peekNext() {
      return null;
    }
    
    @Override
    public void remove() {}

    @Override
    public void close() {}
    
  };
}
