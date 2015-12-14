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

import static org.junit.Assert.fail;
import io.multimap.Callables.Function;
import io.multimap.Callables.Predicate;
import io.multimap.Callables.Procedure;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.file.FileVisitResult;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.SimpleFileVisitor;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.HashSet;
import java.util.Set;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

public class MapTest {
  // These tests are only to ensure that the JNI binding of each method works.
  // Real functional unit tests are implemented in the original C++ code base.

  static final Path DIRECTORY = Paths.get(MapTest.class.getName());
  
  static final Predicate IS_EVEN = new Predicate() {
    @Override
    protected boolean callOnReadOnly(ByteBuffer bytes) {
      return getSuffix(bytes) % 2 == 0;
    }
  };
  
  static final Function NEXT_IF_EVEN = new Function() {
    @Override
    protected byte[] callOnReadOnly(ByteBuffer bytes) {
      int suffix = getSuffix(bytes);
      return (suffix % 2 == 0) ? makeValue(suffix + 1) : null;
    }
  };

  static void deleteDirectoryIfExists(Path directory) throws IOException {
    if (Files.isDirectory(directory)) {
      Files.walkFileTree(directory, new SimpleFileVisitor<Path>() {
        @Override
        public FileVisitResult visitFile(Path file, BasicFileAttributes attrs) throws IOException {
          Files.delete(file);
          return FileVisitResult.CONTINUE;
        }
        @Override
        public FileVisitResult postVisitDirectory(Path dir, IOException exc) throws IOException {
          Files.delete(dir);
          return FileVisitResult.CONTINUE;
        }
      });
    }
  }
  
  static Map createAndFillMap(Path directory, int numKeys, int numValuesPerKey) throws Exception {
    Options options = new Options();
    options.setCreateIfMissing(true);
    Map map = new Map(DIRECTORY, options);
    fill(map, numKeys, numValuesPerKey);
    return map;
  }
  
  static void fill(Map map, int numKeys, int numValuesPerKey) throws Exception {
    for (int i = 0; i < numKeys; ++i) {
      for (int j = 0; j < numValuesPerKey; ++j) {
        map.put(makeKey(i), makeValue(j));
      }
    }
  }

  static byte[] makeKey(int suffix) {
    return makeBytes("key", suffix);
  }

  static byte[] makeValue(int suffix) {
    return makeBytes("value", suffix);
  }
  
  static byte[] makeBytes(String prefix, int suffix) {
    return (prefix + suffix).getBytes();
  }
  
  static int getSuffix(ByteBuffer buffer) {
    String str = new String(toByteArray(buffer));
    if (str.startsWith("key")) {
      return Integer.parseInt(str.substring(3));
    }
    if (str.startsWith("value")) {
      return Integer.parseInt(str.substring(5));
    }
    throw new IllegalArgumentException();
  }
  
  static byte[] toByteArray(ByteBuffer buffer) {
    byte[] bytes = new byte[buffer.capacity()];
    buffer.get(bytes);
    return bytes;
  }
  
  @BeforeClass
  public static void setUpBeforeClass() throws Exception {}

  @AfterClass
  public static void tearDownAfterClass() throws Exception {
    deleteDirectoryIfExists(DIRECTORY);
  }

  @Before
  public void setUp() throws Exception {
    deleteDirectoryIfExists(DIRECTORY);
    Files.createDirectory(DIRECTORY);
  }

  @After
  public void tearDown() throws Exception {}

  @Test (expected = Exception.class)
  public void testMapCtorShouldThrow() throws Exception {
    // The map does not exist and options.createIfMissing is false.
    Map map = new Map(DIRECTORY, new Options());
    map.close();
  }
  
  @Test
  public void testMapCtorNotShouldThrow() throws Exception {
    Options options = new Options();
    options.setCreateIfMissing(true);
    Map map = new Map(DIRECTORY, options);
    map.close();
  }

  @Test
  public void testPut() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    map.close();
  }

  @Test
  public void testGet() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    for (int i = 0; i < numKeys; ++i) {
      Iterator iter = map.get(makeKey(i));
      for (int j = 0; j < numValuesPerKeys; ++j) {
        Assert.assertTrue(iter.hasNext());
        Assert.assertEquals(numValuesPerKeys - j, iter.available());
        Assert.assertArrayEquals(makeValue(j), iter.nextAsByteArray());
      }
      Assert.assertFalse(iter.hasNext());
      Assert.assertEquals(0, iter.available());
      iter.close();
    }
    map.close();
  }

  @Test
  public void testGetMutable() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    for (int i = 0; i < numKeys; ++i) {
      Iterator iter = map.getMutable(makeKey(i));
      for (int j = 0; j < numValuesPerKeys; ++j) {
        Assert.assertTrue(iter.hasNext());
        Assert.assertEquals(numValuesPerKeys - j, iter.available());
        Assert.assertArrayEquals(makeValue(j), iter.nextAsByteArray());
        if (j % 2 == 0) {
          iter.remove();
        }
      }
      Assert.assertFalse(iter.hasNext());
      Assert.assertEquals(0, iter.available());
      iter.close();
    }
    for (int i = 0; i < numKeys; ++i) {
      Iterator iter = map.getMutable(makeKey(i));
      for (int j = 1; j < numValuesPerKeys; j += 2) {
        Assert.assertTrue(iter.hasNext());
        Assert.assertEquals((numValuesPerKeys - j + 1) / 2, iter.available());
        Assert.assertArrayEquals(makeValue(j), iter.nextAsByteArray());
      }
      Assert.assertFalse(iter.hasNext());
      Assert.assertEquals(0, iter.available());
      iter.close();
    }
    map.close();
  }

  @Test
  public void testRemove() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    for (int i = 0; i < numKeys; i += 2) {
      Assert.assertEquals(numValuesPerKeys, map.remove(makeKey(i)));
    }
    for (int i = 0; i < numKeys; ++i) {
      Iterator iter = map.get(makeKey(i));
      if (i % 2 == 0) {
        Assert.assertFalse(iter.hasNext());
        Assert.assertEquals(0, iter.available());
      } else {
        Assert.assertTrue(iter.hasNext());
        Assert.assertEquals(numValuesPerKeys, iter.available());
      }
      iter.close();
    }
    map.close();
  }

  @Test
  public void testRemoveAll() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);

    for (int i = 0; i < numKeys; ++i) {
      long expectedNumRemoved = numValuesPerKeys / 2;
      long actualNumRemoved = map.removeAll(makeKey(i), IS_EVEN);
      Assert.assertEquals(expectedNumRemoved, actualNumRemoved);
    }

    for (int i = 0; i < numKeys; ++i) {
      Iterator iter = map.get(makeKey(i));
      for (int j = 0; j < numValuesPerKeys; ++j) {
        byte[] value = makeValue(j);
        if (IS_EVEN.call(ByteBuffer.wrap(value))) {
          // This value has been removed.
        } else {
          Assert.assertTrue(iter.hasNext());
          Assert.assertArrayEquals(value, iter.nextAsByteArray());
        }
      }
      Assert.assertFalse(iter.hasNext());
      Assert.assertEquals(0, iter.available());
      iter.close();
    }
    map.close();
  }

  @Test
  public void testRemoveAllEqual() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    
    for (int i = 0; i < numKeys; ++i) {
      long expectedNumRemoved = 1;
      long actualNumRemoved = map.removeAllEqual(makeKey(i), makeValue(123));
      Assert.assertEquals(expectedNumRemoved, actualNumRemoved);
    }
    
    for (int i = 0; i < numKeys; ++i) {
      Iterator iter = map.get(makeKey(i));
      for (int j = 0; j < numValuesPerKeys; ++j) {
        if (j == 123) {
          // This value has been removed.
        } else {
          Assert.assertTrue(iter.hasNext());
          Assert.assertArrayEquals(makeValue(j), iter.nextAsByteArray());
        }
      }
      Assert.assertFalse(iter.hasNext());
      Assert.assertEquals(0, iter.available());
      iter.close();
    }
    map.close();
  }

  @Test
  public void testRemoveFirst() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    
    for (int i = 0; i < numKeys; ++i) {
      boolean removed = map.removeFirst(makeKey(i), IS_EVEN);
      Assert.assertTrue(removed);
    }
    
    for (int i = 0; i < numKeys; ++i) {
      Iterator iter = map.get(makeKey(i));
      int j = 0;
      for (; j < numValuesPerKeys; ++j) {
        byte[] value = makeValue(j);
        if (IS_EVEN.call(ByteBuffer.wrap(value))) {
          // This value has been removed.
          break;
        } else {
          Assert.assertTrue(iter.hasNext());
          Assert.assertArrayEquals(value, iter.nextAsByteArray());
        }
      }
      for (++j; j < numValuesPerKeys; ++j) {
        Assert.assertTrue(iter.hasNext());
        Assert.assertArrayEquals(makeValue(j), iter.nextAsByteArray());
      }
      Assert.assertFalse(iter.hasNext());
      Assert.assertEquals(0, iter.available());
      iter.close();
    }
    map.close();
  }

  @Test
  public void testRemoveFirstEqual() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    
    for (int i = 0; i < numKeys; ++i) {
      boolean removed = map.removeFirstEqual(makeKey(i), makeValue(123));
      Assert.assertTrue(removed);
    }
    
    for (int i = 0; i < numKeys; ++i) {
      Iterator iter = map.get(makeKey(i));
      int j = 0;
      for (; j < numValuesPerKeys; ++j) {
        if (j == 123) {
          // This value has been removed.
          break;
        } else {
          Assert.assertTrue(iter.hasNext());
          Assert.assertArrayEquals(makeValue(j), iter.nextAsByteArray());
        }
      }
      for (++j; j < numValuesPerKeys; ++j) {
        Assert.assertTrue(iter.hasNext());
        Assert.assertArrayEquals(makeValue(j), iter.nextAsByteArray());
      }
      Assert.assertFalse(iter.hasNext());
      Assert.assertEquals(0, iter.available());
      iter.close();
    }
    map.close();
  }

  @Test
  public void testReplaceAll() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);

    for (int i = 0; i < numKeys; ++i) {
      long expectedNumReplaced = numValuesPerKeys / 2;
      long actualNumReplaced = map.replaceAll(makeKey(i), NEXT_IF_EVEN);
      Assert.assertEquals(expectedNumReplaced, actualNumReplaced);
    }

    for (int i = 0; i < numKeys; ++i) {
      Iterator iter = map.get(makeKey(i));
      for (int j = 1; j < numValuesPerKeys; j += 2) {
        Assert.assertTrue(iter.hasNext());
        Assert.assertArrayEquals(makeValue(j), iter.nextAsByteArray());
      }
      // Replaced values have been inserted at the end.
      for (int j = 1; j < numValuesPerKeys; j += 2) {
        Assert.assertTrue(iter.hasNext());
        Assert.assertArrayEquals(makeValue(j), iter.nextAsByteArray());
      }
      Assert.assertFalse(iter.hasNext());
      Assert.assertEquals(0, iter.available());
      iter.close();
    }
    map.close();
  }

  @Test
  public void testReplaceAllEqual() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    
    for (int i = 0; i < numKeys; ++i) {
      long expectedNumReplaced = 1;
      long actualNumRemoved = map.replaceAllEqual(makeKey(i), makeValue(123), makeValue(124));
      Assert.assertEquals(expectedNumReplaced, actualNumRemoved);
    }
    
    for (int i = 0; i < numKeys; ++i) {
      Iterator iter = map.get(makeKey(i));
      for (int j = 0; j < numValuesPerKeys; ++j) {
        if (j == 123) {
          // This value has been removed.
        } else {
          Assert.assertTrue(iter.hasNext());
          Assert.assertArrayEquals(makeValue(j), iter.nextAsByteArray());
        }
      }
      // The last element is the replaced value.
      Assert.assertTrue(iter.hasNext());
      Assert.assertArrayEquals(makeValue(124), iter.nextAsByteArray());
      Assert.assertFalse(iter.hasNext());
      Assert.assertEquals(0, iter.available());
      iter.close();
    }
    map.close();
  }

  @Test
  public void testReplaceFirst() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);

    for (int i = 0; i < numKeys; ++i) {
      boolean replaced = map.replaceFirst(makeKey(i), NEXT_IF_EVEN);
      Assert.assertTrue(replaced);
    }

    for (int i = 0; i < numKeys; ++i) {
      Iterator iter = map.get(makeKey(i));
      for (int j = 1; j < numValuesPerKeys; ++j) {
        Assert.assertTrue(iter.hasNext());
        Assert.assertArrayEquals(makeValue(j), iter.nextAsByteArray());
      }
      // The last element is the replaced value.
      Assert.assertTrue(iter.hasNext());
      Assert.assertArrayEquals(makeValue(1), iter.nextAsByteArray());
      Assert.assertFalse(iter.hasNext());
      Assert.assertEquals(0, iter.available());
      iter.close();
    }
    map.close();
  }

  @Test
  public void testReplaceFirstEqual() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    
    for (int i = 0; i < numKeys; ++i) {
      boolean replaced = map.replaceFirstEqual(makeKey(i), makeValue(123), makeValue(124));
      Assert.assertTrue(replaced);
    }
    
    for (int i = 0; i < numKeys; ++i) {
      Iterator iter = map.get(makeKey(i));
      for (int j = 0; j < numValuesPerKeys; ++j) {
        if (j == 123) {
          // This value has been removed.
        } else {
          Assert.assertTrue(iter.hasNext());
          Assert.assertArrayEquals(makeValue(j), iter.nextAsByteArray());
        }
      }
      // The last element is the replaced value.
      Assert.assertTrue(iter.hasNext());
      Assert.assertArrayEquals(makeValue(124), iter.nextAsByteArray());
      Assert.assertFalse(iter.hasNext());
      Assert.assertEquals(0, iter.available());
      iter.close();
    }
    map.close();
  }

  @Test
  public void testForEachKey() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    
    final Set<Integer> keys = new HashSet<>();
    map.forEachKey(new Procedure() {
      @Override
      protected void callOnReadOnly(ByteBuffer bytes) {
        keys.add(getSuffix(bytes));
      }
    });
    
    Assert.assertEquals(numKeys, keys.size());
    for (int i = 0; i < numKeys; ++i) {
      Assert.assertTrue(keys.contains(i));
    }
    
    // TODO Remove some keys
  }

  @Test
  public void testForEachValueByteArrayProcedure() {
    fail("Not yet implemented"); // TODO
  }

  @Test
  public void testForEachValueByteArrayPredicate() {
    fail("Not yet implemented"); // TODO
  }

  @Test
  public void testIsReadOnly() {
    fail("Not yet implemented"); // TODO
  }

  @Test
  public void testClose() {
    fail("Not yet implemented"); // TODO
  }

  @Test
  public void testImportFromBase64PathPath() {
    fail("Not yet implemented"); // TODO
  }

  @Test
  public void testImportFromBase64PathPathOptions() {
    fail("Not yet implemented"); // TODO
  }

  @Test
  public void testExportToBase64PathPath() {
    fail("Not yet implemented"); // TODO
  }

  @Test
  public void testExportToBase64PathPathOptions() {
    fail("Not yet implemented"); // TODO
  }

  @Test
  public void testOptimize() {
    fail("Not yet implemented"); // TODO
  }

}
