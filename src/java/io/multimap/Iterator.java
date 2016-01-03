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
 * This abstract class represents a forward iterator on a list of values. In contrast to the
 * standard {@link java.util.Iterator} interface this class has the following properties:
 * 
 * <ul>
 * <li>Lazy initialization. An iterator does not perform any IO operation until {@link #next()}, 
 * {@link #nextAsByteArray()}, {@link #peekNext()} or {@link #peekNextAsByteArray()} has been
 * called. This is useful in cases where multiple iterators need to be requested first to determine
 * in which order they have to be processed.</li>
 * <li>The iterator can tell the number of remaining values via {@link #available()}.</li>
 * <li>The iterator can emit the next value without moving forward via {@link #peekNext()}.</li>
 * <li>The iterator must be closed if no longer needed to release native resources such as locks.
 * Not closing an iterator that implements a locking mechanism leaves the underlying list in locked
 * state which leads to deadlocks sooner or later.</li>
 * </ul>
 * 
 * Objects of this class hold a reader lock on the associated list so that multiple threads can
 * iterate the list at the same time, but no thread can modify it. If the iterator is not needed
 * anymore {@link #close()} must be called in order to release the reader lock.
 * 
 * @author Martin Trenkmann
 */
public class Iterator implements AutoCloseable {

  private ByteBuffer self;

  protected Iterator() {}

  protected Iterator(ByteBuffer nativePtr) {
    Check.notNull(nativePtr);
    Check.notZero(nativePtr.capacity());
    self = nativePtr;
  }

  /**
   * Returns the number of values left to iterate.
   */
  public long available() {
    return Native.available(self);
  }

  /**
   * Tells whether the iterator has one more value.
   */
  public boolean hasNext() {
    return Native.hasNext(self);
  }

  /**
   * Returns the next value as {@link ByteBuffer}. The returned buffer is a direct one, i.e. it
   * wraps a pointer that points directly into native allocated memory not managed by the Java
   * Garbage Collector. This pointer is only valid as long as the iterator does not move forward.
   * Therefore, the buffer should only be used to immediately parse the original value out of it.
   * Most serialization libraries should be able to consume this byte buffer. If a byte array is
   * required or an independent copy of the value use {@link #nextAsByteArray()} instead.
   */
  public ByteBuffer next() {
    return Native.next(self).asReadOnlyBuffer();
  }

  public ByteBuffer peekNext() {
    return Native.peekNext(self).asReadOnlyBuffer();
  }

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

  @Override
  public void close() {
    if (self != null) {
      Native.close(self);
      self = null;
    }
  }

  @Override
  protected void finalize() throws Throwable {
    if (self != null) {
      System.err.printf("WARNING %s.finalize() calls close()\n", getClass().getName());
      close();
    }
    super.finalize();
  }

  private static class Native {
    static native long available(ByteBuffer self);
    static native boolean hasNext(ByteBuffer self);
    static native ByteBuffer next(ByteBuffer self);
    static native ByteBuffer peekNext(ByteBuffer self);
    static native void close(ByteBuffer self);
  }

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

  };
}
