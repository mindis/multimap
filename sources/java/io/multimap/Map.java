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

import io.multimap.Callables.Function;
import io.multimap.Callables.LessThan;
import io.multimap.Callables.Predicate;
import io.multimap.Callables.Procedure;

import java.nio.ByteBuffer;
import java.nio.file.Path;
import java.util.TreeMap;

// Consider to use ByteBuffer instead of byte[] where suitable and/or faster.
public class Map implements AutoCloseable {
  
  static {
    final String LIBNAME = "multimap-jni";
    try {
      System.loadLibrary(LIBNAME);
    } catch (Exception e) {
      System.err.print(e);
    }
  }
  
  static class ImmutableListIter extends Iterator {

    private ByteBuffer self;
    
    private ImmutableListIter(ByteBuffer nativePtr) {
      Check.notNull(nativePtr);
      Check.notZero(nativePtr.capacity());
      self = nativePtr;
    }
    
    @Override
    public long numValues() {
      return Native.numValues(self);
    }

    @Override
    public void seekToFirst() {
      Native.seekToFirst(self);
    }

    @Override
    public void seekTo(byte[] target) {
      Check.notNull(target);
      Native.seekTo(self, target);
    }

    @Override
    public void seekTo(Predicate predicate) {
      Check.notNull(predicate);
      Native.seekTo(self, predicate);
    }

    @Override
    public boolean hasValue() {
      return Native.hasValue(self);
    }

    @Override
    public ByteBuffer getValue() {
      return Native.getValue(self).asReadOnlyBuffer();
    }

    @Override
    public void deleteValue() {
      throw new UnsupportedOperationException();
    }

    @Override
    public void next() {
      Native.next(self);
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
        System.err.printf(
            "WARNING %s.finalize() calls release(), but should be called by the client.\n",
            getClass().getName());
        close();
      }
      super.finalize();
    }
    
    static class Native {
      static native long numValues(ByteBuffer self);
      static native void seekToFirst(ByteBuffer self);
      static native void seekTo(ByteBuffer self, byte[] target);
      static native void seekTo(ByteBuffer self, Predicate predicate);
      static native boolean hasValue(ByteBuffer self);
      static native ByteBuffer getValue(ByteBuffer self);
      static native void next(ByteBuffer self);
      static native void close(ByteBuffer self);
    }
  }
  
  static class MutableListIter extends Iterator {

    private ByteBuffer self;
    
    private MutableListIter(ByteBuffer nativePtr) {
      Check.notNull(nativePtr);
      Check.notZero(nativePtr.capacity());
      self = nativePtr;
    }
    
    @Override
    public long numValues() {
      return Native.numValues(self);
    }

    @Override
    public void seekToFirst() {
      Native.seekToFirst(self);
    }

    @Override
    public void seekTo(byte[] target) {
      Check.notNull(target);
      Native.seekTo(self, target);
    }

    @Override
    public void seekTo(Predicate predicate) {
      Check.notNull(predicate);
      Native.seekTo(self, predicate);
    }

    @Override
    public boolean hasValue() {
      return Native.hasValue(self);
    }

    @Override
    public ByteBuffer getValue() {
      return Native.getValue(self).asReadOnlyBuffer();
    }

    @Override
    public void deleteValue() {
      Native.deleteValue(self);
    }

    @Override
    public void next() {
      Native.next(self);
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
        System.err.printf(
            "WARNING %s.finalize() calls release(), but should be called by the client.\n",
            getClass().getName());
        close();
      }
      super.finalize();
    }
    
    static class Native {
      static native long numValues(ByteBuffer self);
      static native void seekToFirst(ByteBuffer self);
      static native void seekTo(ByteBuffer self, byte[] target);
      static native void seekTo(ByteBuffer self, Predicate predicate);
      static native boolean hasValue(ByteBuffer self);
      static native ByteBuffer getValue(ByteBuffer self);
      static native void deleteValue(ByteBuffer self);
      static native void next(ByteBuffer self);
      static native void close(ByteBuffer self);
    }
  }
  
  private ByteBuffer self;
  
  private Map(ByteBuffer nativePtr) {
    Check.notNull(nativePtr);
    Check.notZero(nativePtr.capacity());
    self = nativePtr;
  }
  
  public Map(Path directory, Options options) throws Exception {
    this(Native.open(directory.toString(), options.toString()));
  }
  
  public void close() {
    if (self != null) {
      Native.close(self);
      self = null;        
    }
  }
  
  public void put(byte[] key, byte[] value) {
    Check.notNull(key);
    Check.notNull(value);
    Native.put(self, key, value);
  }
  
  @SuppressWarnings("resource")
  public Iterator get(byte[] key) {
    Check.notNull(key);
    ByteBuffer nativePtr = Native.get(self, key);
    return (nativePtr != null) ? new ImmutableListIter(nativePtr) : Iterator.EMPTY;
  }
  
  @SuppressWarnings("resource")
  public Iterator getMutable(byte[] key) {
    Check.notNull(key);
    ByteBuffer nativePtr = Native.getMutable(self, key);
    return (nativePtr != null) ? new MutableListIter(nativePtr) : Iterator.EMPTY;
  }
  
  public boolean contains(byte[] key) {
    return Native.contains(self, key);
  }
  
  public long delete(byte[] key) {
    Check.notNull(key);
    return Native.delete(self, key);
  }
  
  public boolean deleteFirst(byte[] key, Predicate predicate) {
    Check.notNull(key);
    Check.notNull(predicate);
    return Native.deleteFirst(self, key, predicate);
  }
  
  public boolean deleteFirstEqual(byte[] key, byte[] value) {
    Check.notNull(key);
    Check.notNull(value);
    return Native.deleteFirstEqual(self, key, value);
  }
  
  public long deleteAll(byte[] key, Predicate predicate) {
    Check.notNull(key);
    Check.notNull(predicate);
    return Native.deleteAll(self, key, predicate);
  }
  
  public long deleteAllEqual(byte[] key, byte[] value) {
    Check.notNull(key);
    Check.notNull(value);
    return Native.deleteAllEqual(self, key, value);
  }
  
  public boolean replaceFirst(byte[] key, Function function) {
    Check.notNull(key);
    Check.notNull(function);
    return Native.replaceFirst(self, key, function);
  }
  
  public boolean replaceFirstEqual(byte[] key, byte[] oldValue, byte[] newValue) {
    Check.notNull(key);
    Check.notNull(oldValue);
    Check.notNull(newValue);
    return Native.replaceFirstEqual(self, key, oldValue, newValue);
  }
  
  public long replaceAll(byte[] key, Function function) {
    Check.notNull(key);
    Check.notNull(function);
    return Native.replaceAll(self, key, function);
  }
  
  public long replaceAllEqual(byte[] key, byte[] oldValue, byte[] newValue) {
    Check.notNull(key);
    Check.notNull(oldValue);
    Check.notNull(newValue);
    return Native.replaceAllEqual(self, key, oldValue, newValue);
  }
  
  public void forEachKey(Procedure procedure) {
    Check.notNull(procedure);
    Native.forEachKey(self, procedure);
  }
  
  public java.util.Map<String, String> getProperties() {
    java.util.Map<String, String> properties = new TreeMap<>();
    String[] lines = Native.getProperties(self).split("\n");
    for (String line : lines) {
      String[] property = line.split("=");
      properties.put(property[0], property[1]);
    }
    return properties;
  }
  
  public void printProperties() {
    Native.printProperties(self);
  }
  
  @Override
  protected void finalize() throws Throwable {
    if (self != null) {
      System.err.printf(
          "WARNING %s.finalize() calls close(), but should be called by the client.\n",
          getClass().getName());
      close();
    }
    super.finalize();
  }
    
  public static void copy(Path from, Path to) {}
  
  public static void copy(Path from, Path to, short newBlockSize) {}
  
  public static void copy(Path from, Path to, LessThan lessThan) {}
  
  public static void copy(Path from, Path to, LessThan lessThan, short newBlockSize) {}
  
  static class Native {
    static native ByteBuffer open(String directory, String options) throws Exception;
    static native void close(ByteBuffer self);
    static native void put(ByteBuffer self, byte[] key, byte[] value);
    static native ByteBuffer get(ByteBuffer self, byte[] key);
    static native ByteBuffer getMutable(ByteBuffer self, byte[] key);
    static native boolean contains(ByteBuffer self, byte[] key);
    static native long delete(ByteBuffer self, byte[] key);
    static native boolean deleteFirst(ByteBuffer self, byte[] key, Predicate predicate);
    static native boolean deleteFirstEqual(ByteBuffer self, byte[] key, byte[] value);
    static native long deleteAll(ByteBuffer self, byte[] key, Predicate predicate);
    static native long deleteAllEqual(ByteBuffer self, byte[] key, byte[] value);
    static native boolean replaceFirst(ByteBuffer self, byte[] key, Function function);
    static native boolean replaceFirstEqual(ByteBuffer self, byte[] key, byte[] oldValue, byte[] newValue);
    static native long replaceAll(ByteBuffer self, byte[] key, Function function);
    static native long replaceAllEqual(ByteBuffer self, byte[] key, byte[] oldValue, byte[] newValue);
    static native void forEachKey(ByteBuffer self, Procedure procedure);
    static native String getProperties(ByteBuffer self);  
    static native void printProperties(ByteBuffer self);
    
    static native void copy(String from, String to);
    static native void copy(String from, String to, short newBlockSize);
    static native void copy(String from, String to, LessThan lessThan);
    static native void copy(String from, String to, LessThan lessThan, short newBlockSize);
  }
    
  // http://journals.ecs.soton.ac.uk/java/tutorial/native1.1/implementing/index.html
  // https://www.ibm.com/developerworks/library/j-jni/
}
