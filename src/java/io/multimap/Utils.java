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
 * This class provides a collection of utility functions.
 * 
 * @author Martin Trenkmann
 * @since 0.4.0
 */
public class Utils {

  public static final Charset UTF8 = Charset.forName("UTF-8");
  
  /**
   * Converts a byte buffer to a byte array.
   */
  public static byte[] toByteArray(ByteBuffer bytes) {
    byte[] array = new byte[bytes.capacity()];
    bytes.get(array);
    return array;
  }
  
  /**
   * Converts a byte buffer to a string using UTF-8 as the character set for decoding.
   */
  public static String toString(ByteBuffer bytes) {
    return toString(toByteArray(bytes));
  }
  
  /**
   * Converts a byte array to a string using UTF-8 as the character set for decoding.
   */
  public static String toString(byte[] bytes) {
    return new String(bytes, UTF8);
  }
  
}
