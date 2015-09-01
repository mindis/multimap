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

public final class Callables {
  
  private Callables() {}

  public static abstract class Procedure {

    public void call(ByteBuffer key) {
      callOnReadOnly(key.asReadOnlyBuffer());
    }

    public abstract void callOnReadOnly(ByteBuffer key);
  }

  public static abstract class Predicate {

    public boolean call(ByteBuffer value) {
      return callOnReadOnly(value.asReadOnlyBuffer());
    }

    public abstract boolean callOnReadOnly(ByteBuffer value);
  }

  public static abstract class Function {

    public byte[] call(ByteBuffer value) {
      return callOnReadOnly(value.asReadOnlyBuffer());
    }

    public abstract byte[] callOnReadOnly(ByteBuffer value);
  }

  public static abstract class LessThan {

    public boolean call(ByteBuffer a, ByteBuffer b) {
      return callOnReadOnly(a.asReadOnlyBuffer(), b.asReadOnlyBuffer());
    }

    public abstract boolean callOnReadOnly(ByteBuffer a, ByteBuffer b);
  }

}
