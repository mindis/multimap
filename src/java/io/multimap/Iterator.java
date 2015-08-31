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

public abstract class Iterator implements AutoCloseable {

  public abstract long numValues();

  public abstract void seekToFirst();

  public abstract void seekTo(byte[] target);

  public abstract void seekTo(Predicate predicate);

  public abstract boolean hasValue();

  // TODO Document why we return ByteBuffer rather than byte[].
  // - byte[] is a newly allocated object managed by Java GC, but it only serves as a temporary
  // because its content will be parsed into some rich object immediately in most cases.
  // - ByteBuffer points directly into some native buffer on C++ side. Its content is valid as long
  // as the iterator is not moved forward. Storing the raw bytes requires a deep copy into a
  // user allocated byte[]. ByteBuffer is a view.
  // The returned ByteBuffer should be read-only.
  public abstract ByteBuffer getValue();

  // Returns a deep copy.
  public byte[] getValueAsByteArray() {
    ByteBuffer value = getValue();
    byte[] copy = new byte[value.capacity()];
    value.get(copy);
    return copy;
  }

  public abstract void deleteValue();

  public abstract void next();

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