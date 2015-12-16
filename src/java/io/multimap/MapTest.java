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
import io.multimap.Callables.Predicate;
import io.multimap.Callables.Procedure;

import java.io.BufferedWriter;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.file.FileVisitResult;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.SimpleFileVisitor;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.Base64;
import java.util.Base64.Encoder;
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
  static final Path DIRECTORY2 = Paths.get(MapTest.class.getName() + 2);
  static final Path DATAFILE = DIRECTORY.resolve("data.csv");
  
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
    String key = "key";
    if (str.startsWith(key)) {
      return Integer.parseInt(str.substring(key.length()));
    }
    String value = "value";
    if (str.startsWith(value)) {
      return Integer.parseInt(str.substring(value.length()));
    }
    throw new IllegalArgumentException();
  }
  
  static byte[] toByteArray(ByteBuffer buffer) {
    byte[] bytes = new byte[buffer.capacity()];
    buffer.get(bytes);
    return bytes;
  }
  
  static void writeAsBase64ToFile(Path datafile, int numKeys, int numValuesPerKeys) throws IOException {
    BufferedWriter bw = Files.newBufferedWriter(datafile);
    Encoder enc = Base64.getEncoder();
    for (int i = 0; i < numKeys; ++i) {
      bw.write(enc.encodeToString(makeKey(i)));
      for (int j = 0; j < numValuesPerKeys; ++j) {
        bw.append(' ').write(enc.encodeToString(makeValue(j)));
      }
      bw.newLine();
    }
    bw.close();
  }
  
  @BeforeClass
  public static void setUpBeforeClass() throws Exception {}

  @AfterClass
  public static void tearDownAfterClass() throws Exception {
    deleteDirectoryIfExists(DIRECTORY);
    deleteDirectoryIfExists(DIRECTORY2);
  }

  @Before
  public void setUp() throws Exception {
    deleteDirectoryIfExists(DIRECTORY);
    deleteDirectoryIfExists(DIRECTORY2);
    Files.createDirectory(DIRECTORY);
    Files.createDirectory(DIRECTORY2);
  }

  @After
  public void tearDown() throws Exception {
    Files.deleteIfExists(DATAFILE);
  }

  @Test (expected = Exception.class)
  public void testMapCtorShouldThrow() throws Exception {
    // The map does not exist and options.createIfMissing is false.
    Map map = new Map(DIRECTORY);
    map.close();
  }
  
  @Test
  public void testMapCtorShouldNotThrow() throws Exception {
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
  public void testRemoveKey() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    for (int i = 0; i < numKeys; i += 2) {
      Assert.assertTrue(map.removeKey(makeKey(i)));
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
  public void testRemoveKeys() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    
    map.removeKeys(IS_EVEN);
    for (int i = 0; i < numKeys; ++i) {
      if (i % 2 == 0) {
        Assert.assertFalse(map.containsKey(makeKey(i)));
      } else {
        Assert.assertTrue(map.containsKey(makeKey(i)));
      }
    }
    map.close();
  }

  @Test
  public void testRemoveValue() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    
    for (int i = 0; i < numKeys; ++i) {
      boolean removed = map.removeValue(makeKey(i), makeValue(123));
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
  public void testRemoveValueViaPredicate() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    
    for (int i = 0; i < numKeys; ++i) {
      boolean removed = map.removeValue(makeKey(i), IS_EVEN);
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
  public void testRemoveValues() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    
    for (int i = 0; i < numKeys; ++i) {
      long expectedNumRemoved = 1;
      long actualNumRemoved = map.removeValues(makeKey(i), makeValue(123));
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
  public void testRemoveValuesViaPredicate() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);

    for (int i = 0; i < numKeys; ++i) {
      long expectedNumRemoved = numValuesPerKeys / 2;
      long actualNumRemoved = map.removeValues(makeKey(i), IS_EVEN);
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
  public void testReplaceValue() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    
    for (int i = 0; i < numKeys; ++i) {
      boolean replaced = map.replaceValue(makeKey(i), makeValue(123), makeValue(124));
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
  public void testReplaceValueViaFunction() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);

    for (int i = 0; i < numKeys; ++i) {
      boolean replaced = map.replaceValue(makeKey(i), NEXT_IF_EVEN);
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
  public void testReplaceValues() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    
    for (int i = 0; i < numKeys; ++i) {
      long expectedNumReplaced = 1;
      long actualNumRemoved = map.replaceValues(makeKey(i), makeValue(123), makeValue(124));
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
  public void testReplaceValuesViaFunction() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);

    for (int i = 0; i < numKeys; ++i) {
      long expectedNumReplaced = numValuesPerKeys / 2;
      long actualNumReplaced = map.replaceValues(makeKey(i), NEXT_IF_EVEN);
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
  public void testForEachKey() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    
    // Collect the whole key set.
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
    
    // Remove every second key and collect keys again.
    for (int i = 0; i < numKeys; i += 2) {
      Assert.assertTrue(map.removeKey(makeKey(i)));
    }
    keys.clear();
    map.forEachKey(new Procedure() {
      @Override
      protected void callOnReadOnly(ByteBuffer bytes) {
        keys.add(getSuffix(bytes));
      }
    });
    Assert.assertEquals(numKeys / 2, keys.size());
    for (int i = 1; i < numKeys; i += 2) {
      Assert.assertTrue(keys.contains(i));
    }
    map.close();
  }

  @Test
  public void testForEachValue() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    
    // Collect all values associated with "23".
    byte[] key = makeKey(23);
    final Set<Integer> values = new HashSet<>();
    map.forEachValue(key, new Procedure() {
      @Override
      protected void callOnReadOnly(ByteBuffer bytes) {
        values.add(getSuffix(bytes));
      }
    });
    Assert.assertEquals(numValuesPerKeys, values.size());
    for (int i = 0; i < numValuesPerKeys; ++i) {
      Assert.assertTrue(values.contains(i));
    }
    
    // Remove every second value and collect values again.
    map.removeValues(key, IS_EVEN);
    values.clear();
    map.forEachValue(key, new Procedure() {
      @Override
      protected void callOnReadOnly(ByteBuffer bytes) {
        values.add(getSuffix(bytes));
      }
    });
    Assert.assertEquals(numValuesPerKeys / 2, values.size());
    for (int i = 1; i < numValuesPerKeys; i += 2) {
      Assert.assertTrue(values.contains(i));
    }
    map.close();
  }

  @Test
  public void testIsReadOnly() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    Assert.assertFalse(map.isReadOnly());
    map.close();
    
    Options options = new Options();
    options.setReadonly(true);
    map = new Map(DIRECTORY, options);
    Assert.assertTrue(map.isReadOnly());
    map.close();
  }

  @Test
  public void testClose() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    Map map = createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys);
    map.close();
    map.close(); // Double close should have no effect.
  }

  @Test (expected = Exception.class)
  public void testImportFromBase64ShouldThrow() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    writeAsBase64ToFile(DATAFILE, numKeys, numValuesPerKeys);
    Map.importFromBase64(DIRECTORY, DATAFILE);
  }

  @Test
  public void testImportFromBase64ShouldNotThrow() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    writeAsBase64ToFile(DATAFILE, numKeys, numValuesPerKeys);
    
    Options options = new Options();
    options.setCreateIfMissing(true);
    Map.importFromBase64(DIRECTORY, DATAFILE, options);
    
    Map map = new Map(DIRECTORY);
    for (int i = 0; i < numKeys; ++i) {
      Iterator iter = map.get(makeKey(i));
      for (int j = 0; j < numValuesPerKeys; ++j) {
        Assert.assertTrue(iter.hasNext());
        Assert.assertEquals(j, getSuffix(iter.next()));
      }
      iter.close();
    }
    map.close();
  }

  @Test (expected = Exception.class)
  public void testExportToBase64ShouldThrow() throws Exception {
    Map.exportToBase64(DIRECTORY, DATAFILE);
  }

  @Test
  public void testExportToBase64ShouldNotThrow() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys).close();
    Map.exportToBase64(DIRECTORY, DATAFILE);

    // Clear the map.
    Map map = new Map(DIRECTORY);
    Assert.assertEquals(numKeys, map.removeKeys(new Predicate() {
      @Override
      protected boolean callOnReadOnly(ByteBuffer bytes) {
        return true;
      }
    }));
    map.close();

    // Import and check content.
    Map.importFromBase64(DIRECTORY, DATAFILE);
    map = new Map(DIRECTORY);
    for (int i = 0; i < numKeys; ++i) {
      Iterator iter = map.get(makeKey(i));
      for (int j = 0; j < numValuesPerKeys; ++j) {
        Assert.assertTrue(iter.hasNext());
        Assert.assertEquals(j, getSuffix(iter.next()));
      }
      iter.close();
    }
    map.close();
  }

  @Test
  public void testExportToBase64FromEmptyMapShouldNotThrow() throws Exception {
    createAndFillMap(DIRECTORY, 0, 0).close();
    Map.exportToBase64(DIRECTORY, DATAFILE);
  }

  @Test (expected = Exception.class)
  public void testOptimizeShouldThrowIfSourceNotExists() throws Exception {
    Map.optimize(DIRECTORY, DIRECTORY2);
  }
  
  @Test (expected = Exception.class)
  public void testOptimizeShouldThrowIfTargetNotExists() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys).close();
    deleteDirectoryIfExists(DIRECTORY2);
    Map.optimize(DIRECTORY, DIRECTORY2);
  }
  
  @Test (expected = Exception.class)
  public void testOptimizeShouldThrowIfSourceAndTargetReferSameDirectory() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys).close();
    Map.optimize(DIRECTORY, DIRECTORY);
  }

  @Test
  public void testOptimizeShouldNotThrow() throws Exception {
    int numKeys = 1000;
    int numValuesPerKeys = 1000;
    createAndFillMap(DIRECTORY, numKeys, numValuesPerKeys).close();
    Map.optimize(DIRECTORY, DIRECTORY2);

    Map map = new Map(DIRECTORY2);
    for (int i = 0; i < numKeys; ++i) {
      Iterator iter = map.get(makeKey(i));
      for (int j = 0; j < numValuesPerKeys; ++j) {
        Assert.assertTrue(iter.hasNext());
        Assert.assertEquals(j, getSuffix(iter.next()));
      }
      iter.close();
    }
    map.close();
  }

}
