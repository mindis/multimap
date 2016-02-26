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
import java.nio.charset.Charset;

/**
 * This class provides utility classes and functions.
 * 
 * @author Martin Trenkmann
 * @since 0.4.0
 */
public class Utils {

  private Utils() {}
  
  /**
   * An UTF-8 charset constant used for encoding/decoding strings to/from byte arrays.
   */
  public static final Charset UTF8 = Charset.forName("UTF-8");
  
  /**
   * Copies the byte buffer's content to a newly allocated byte array.
   */
  public static byte[] toByteArray(ByteBuffer bytes) {
    byte[] array = new byte[bytes.capacity()];
    bytes.get(array);
    return array;
  }
  
  /**
   * Serializes the string's content to an UTF-8 encoded byte array.
   */
  public static byte[] toByteArray(String string) {
    return string.getBytes(UTF8);
  }
  
  /**
   * Parses a string out of a byte buffer whose content is UTF-8 encoded.
   */
  public static String toString(ByteBuffer bytes) {
    return toString(toByteArray(bytes));
  }
  
  /**
   * Parses a string out of a byte array whose content is UTF-8 encoded.
   */
  public static String toString(byte[] bytes) {
    return new String(bytes, UTF8);
  }
  
  /**
   * A class that can hold two values of different type. Instances are immutable.
   * 
   * @author Martin Trenkmann
   * @since 0.5.0
   */
  public static class Pair<A, B> {
    
    private final A a;
    private final B b;
    
    public Pair(A a, B b) {
      this.a = a;
      this.b = b;
    }
    
    /**
     * Returns the first element.
     */
    public A a() { return a; }
    
    /**
     * Returns the second element.
     */
    public B b() { return b; }
  }
  
}
