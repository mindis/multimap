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

/**
 * This class provides static helper functions.
 * 
 * @author Martin Trenkmann
 */
public final class Check {

  public static final void notNull(Object object) {
    if (object == null) {
      throw new AssertionError();
    }
  }

  public static final void notZero(long value) {
    if (value == 0) {
      throw new AssertionError();
    }
  }

  public static void isPositive(int value) {
    if (value < 0) {
      throw new AssertionError();
    }
  }

  public static void isPositive(long value) {
    if (value < 0) {
      throw new AssertionError();
    }
  }

}
