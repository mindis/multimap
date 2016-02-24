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
 * An iterator that reads a list of values, possibly by streaming them from an external device.
 * Objects of this class hold a reader lock on the associated list so that multiple threads can
 * iterate the same list at the same time, but no thread can modify it. If an instance is not
 * needed anymore {@link #close()} must be called in order to release the reader lock. Not closing
 * an iterator leaves the underlying list in locked state which leads to deadlocks sooner or later.
 * 
 * <p>In contrast to the standard {@link java.util.Iterator} interface, this class has the
 * following properties:</p>
 * 
 * <ul>
 * <li>Lazy initialization. The iterator does not perform any I/O operation until a value is
 * actually requested via {@link #next()}, {@link #nextAsByteArray()}, {@link #peekNext()}, or
 * {@link #peekNextAsByteArray()}. This might be useful in cases where multiple iterators have to
 * be collected first in order to decide in which order they need to be processed.</li>
 * <li>The iterator can tell the number of remaining values via {@link #available()}.</li>
 * <li>The iterator can emit the next value without moving forward via {@link #peekNext()} or
 * {@link #peekNextAsByteArray()}.</li>
 * </ul>
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
   * Returns the remaining number of values to be iterated. Calling this method does not trigger
   * any I/O operation.
   */
  public long available() {
    return Native.available(self);
  }

  /**
   * Returns {@code true} if the iterator has more values, {@code false} otherwise.
   */
  public boolean hasNext() {
    return Native.hasNext(self);
  }

  /**
   * Returns the next value from the underlying list and moves the iterator once forward. The
   * returned {@link ByteBuffer} is a direct one, i.e. it wraps a pointer to native allocated
   * memory not managed by the Java Garbage Collector. This pointer is only valid as long as the
   * iterator does not move forward. Therefore, the buffer should only be used to immediately parse
   * the original data out of it. Most serialization libraries should be able to consume this kind
   * of buffer. If a byte array is required or an independent copy of the value
   * {@link #nextAsByteArray()} should be used instead.
   */
  public ByteBuffer next() {
    return Native.next(self).asReadOnlyBuffer();
  }
  
  /**
   * Same as before, but returns the next value as byte array. Internally calls {@link #next()} and
   * copies the content of the byte buffer into a newly allocated byte array. Since that byte array
   * is managed by the Java Garbage Collector it can be copied around or stored in a collection as
   * any other Java object, in contrast to the original byte buffer.
   */
  public byte[] nextAsByteArray() {
    return Utils.toByteArray(next());
  }
  
  /**
   * Same as {@link #next()}, but does not move the iterator once forward.
   */
  public ByteBuffer peekNext() {
    return Native.peekNext(self).asReadOnlyBuffer();
  }

  /**
   * Same as {@link #nextAsByteArray()}, but does not move the iterator once forward.
   */
  public byte[] peekNextAsByteArray() {
    return Utils.toByteArray(peekNext());
  }

  /**
   * Closes the iterator and releases native resources such as locks. An iterator must be closed
   * if no longer needed in order to avoid deadlocks.
   */
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
