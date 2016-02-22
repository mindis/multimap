// This file is part of Multimap.  http://multimap.io
//
// Copyright (C) 2015-2016  Martin Trenkmann
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "multimap/jni/generated/io_multimap_Map_Native.h"

#include "multimap/jni/common.hpp"
#include "multimap/callables.hpp"
#include "multimap/Map.hpp"

// Bug in javah.
// java version "1.7.0_79"
// OpenJDK Runtime Environment (IcedTea 2.5.6) (7u79-2.5.6-1~deb7u1)
// OpenJDK 64-Bit Server VM (build 24.79-b02, mixed mode)
//
// Generated function names with inner Java classes are possibly wrong.
//
// Example:
// For io.multimap.Map.Native.removeValue(ByteBuffer, Callables.Predicate)
// the following function name is generated:
//
// Java_io_multimap_Map_00024Native_removeValue__Ljava_nio_ByteBuffer_2_3BLio_multimap_Callables_Predicate_2
//
// However, it should be:
//
// Java_io_multimap_Map_00024Native_removeValue__Ljava_nio_ByteBuffer_2_3BLio_multimap_Callables_00024Predicate_2
//                                                                                               ^^^^^
// Fix: find/replace by hand or via tool such as sed.

namespace {

inline multimap::Map* getMapPtrFromByteBuffer(JNIEnv* env, jobject buffer) {
  return multimap::jni::getPtrFromByteBuffer<multimap::Map>(env, buffer);
}

}  // namespace

/*
 * Class:     io_multimap_Map_Native
 * Method:    newMap
 * Signature: (Ljava/lang/String;Lio/multimap/Options;)Ljava/nio/ByteBuffer;
 */
JNIEXPORT jobject JNICALL
Java_io_multimap_Map_00024Native_newMap(JNIEnv* env, jclass, jstring jdirectory,
                                        jobject joptions) {
  const auto directory = multimap::jni::makeString(env, jdirectory);
  const auto options = multimap::jni::makeMapOptions(env, joptions);
  try {
    const auto map = new multimap::Map(directory, options);
    return multimap::jni::newByteBufferFromPtr(env, map);
  } catch (std::exception& error) {
    multimap::jni::throwJavaException(env, error.what());
  }
  return nullptr;
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    put
 * Signature: (Ljava/nio/ByteBuffer;[B[B)V
 */
JNIEXPORT void JNICALL
Java_io_multimap_Map_00024Native_put(JNIEnv* env, jclass, jobject self,
                                     jbyteArray jkey, jbyteArray jvalue) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  multimap::jni::BytesRaiiHelper value(env, jvalue);
  try {
    getMapPtrFromByteBuffer(env, self)->put(key.get(), value.get());
  } catch (std::exception& error) {
    multimap::jni::throwJavaException(env, error.what());
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    get
 * Signature: (Ljava/nio/ByteBuffer;[B)Ljava/nio/ByteBuffer;
 */
JNIEXPORT jobject JNICALL
Java_io_multimap_Map_00024Native_get(JNIEnv* env, jclass, jobject self,
                                     jbyteArray jkey) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  auto iter = getMapPtrFromByteBuffer(env, self)->get(key.get());
  if (iter->hasNext()) {
    return multimap::jni::newByteBufferFromPtr(env, iter.release());
  }
  return nullptr;
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    contains
 * Signature: (Ljava/nio/ByteBuffer;[B)Z
 */
JNIEXPORT jboolean JNICALL
Java_io_multimap_Map_00024Native_contains(JNIEnv* env, jclass, jobject self,
                                          jbyteArray jkey) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  return getMapPtrFromByteBuffer(env, self)->contains(key.get());
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    remove
 * Signature: (Ljava/nio/ByteBuffer;[B)I
 */
JNIEXPORT jint JNICALL
Java_io_multimap_Map_00024Native_remove(JNIEnv* env, jclass, jobject self,
                                        jbyteArray jkey) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  return getMapPtrFromByteBuffer(env, self)->remove(key.get());
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    removeOne
 * Signature: (Ljava/nio/ByteBuffer;Lio/multimap/Callables/Predicate;)I
 */
JNIEXPORT jint JNICALL
Java_io_multimap_Map_00024Native_removeOne__Ljava_nio_ByteBuffer_2Lio_multimap_Callables_00024Predicate_2(
    JNIEnv* env, jclass, jobject self, jobject jpredicate) {
  try {
    return getMapPtrFromByteBuffer(env, self)
        ->removeOne(multimap::jni::JavaPredicate(env, jpredicate));
  } catch (std::exception& error) {
    multimap::jni::propagateOrRethrow(env, error);
    return 0;
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    removeAll
 * Signature: (Ljava/nio/ByteBuffer;Lio/multimap/Callables/Predicate;)[B
 */
JNIEXPORT jbyteArray JNICALL
Java_io_multimap_Map_00024Native_removeAll__Ljava_nio_ByteBuffer_2Lio_multimap_Callables_00024Predicate_2(
    JNIEnv* env, jclass, jobject self, jobject jpredicate) {
  try {
    const auto pair = getMapPtrFromByteBuffer(env, self)->removeAll(
        multimap::jni::JavaPredicate(env, jpredicate));
    const auto sizeof_first = sizeof pair.first;
    const auto sizeof_second = sizeof pair.second;
    const auto array = env->NewByteArray(sizeof_first + sizeof_second);
    mt::Check::notNull(array, "NewByteArray() failed");
    env->SetByteArrayRegion(array, 0, sizeof_first,
                            reinterpret_cast<const jbyte*>(&pair.first));
    env->SetByteArrayRegion(array, sizeof_first, sizeof_second,
                            reinterpret_cast<const jbyte*>(&pair.second));
    return array;
  } catch (std::exception& error) {
    multimap::jni::propagateOrRethrow(env, error);
    return nullptr;
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    removeOne
 * Signature: (Ljava/nio/ByteBuffer;[B[B)Z
 */
JNIEXPORT jboolean JNICALL
Java_io_multimap_Map_00024Native_removeOne__Ljava_nio_ByteBuffer_2_3B_3B(
    JNIEnv* env, jclass, jobject self, jbyteArray jkey, jbyteArray jvalue) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  multimap::jni::BytesRaiiHelper value(env, jvalue);
  return getMapPtrFromByteBuffer(env, self)
      ->removeOne(key.get(), multimap::Equal(value.get()));
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    removeOne
 * Signature: (Ljava/nio/ByteBuffer;[BLio/multimap/Callables/Predicate;)Z
 */
JNIEXPORT jboolean JNICALL
Java_io_multimap_Map_00024Native_removeOne__Ljava_nio_ByteBuffer_2_3BLio_multimap_Callables_00024Predicate_2(
    JNIEnv* env, jclass, jobject self, jbyteArray jkey, jobject jpredicate) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  try {
    return getMapPtrFromByteBuffer(env, self)
        ->removeOne(key.get(), multimap::jni::JavaPredicate(env, jpredicate));
  } catch (std::exception& error) {
    multimap::jni::propagateOrRethrow(env, error);
    return false;
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    removeAll
 * Signature: (Ljava/nio/ByteBuffer;[B[B)I
 */
JNIEXPORT jint JNICALL
Java_io_multimap_Map_00024Native_removeAll__Ljava_nio_ByteBuffer_2_3B_3B(
    JNIEnv* env, jclass, jobject self, jbyteArray jkey, jbyteArray jvalue) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  multimap::jni::BytesRaiiHelper value(env, jvalue);
  return getMapPtrFromByteBuffer(env, self)
      ->removeAll(key.get(), multimap::Equal(value.get()));
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    removeAll
 * Signature: (Ljava/nio/ByteBuffer;[BLio/multimap/Callables/Predicate;)I
 */
JNIEXPORT jint JNICALL
Java_io_multimap_Map_00024Native_removeAll__Ljava_nio_ByteBuffer_2_3BLio_multimap_Callables_00024Predicate_2(
    JNIEnv* env, jclass, jobject self, jbyteArray jkey, jobject jpredicate) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  try {
    return getMapPtrFromByteBuffer(env, self)
        ->removeAll(key.get(), multimap::jni::JavaPredicate(env, jpredicate));
  } catch (std::exception& error) {
    multimap::jni::propagateOrRethrow(env, error);
    return 0;
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    replaceOne
 * Signature: (Ljava/nio/ByteBuffer;[B[B[B)Z
 */
JNIEXPORT jboolean JNICALL
Java_io_multimap_Map_00024Native_replaceOne__Ljava_nio_ByteBuffer_2_3B_3B_3B(
    JNIEnv* env, jclass, jobject self, jbyteArray jkey, jbyteArray jold_value,
    jbyteArray jnew_value) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  multimap::jni::BytesRaiiHelper old_value(env, jold_value);
  multimap::jni::BytesRaiiHelper new_value(env, jnew_value);
  return getMapPtrFromByteBuffer(env, self)
      ->replaceOne(key.get(), old_value.get(), new_value.get());
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    replaceOne
 * Signature: (Ljava/nio/ByteBuffer;[BLio/multimap/Callables/Function;)Z
 */
JNIEXPORT jboolean JNICALL
Java_io_multimap_Map_00024Native_replaceOne__Ljava_nio_ByteBuffer_2_3BLio_multimap_Callables_00024Function_2(
    JNIEnv* env, jclass, jobject self, jbyteArray jkey, jobject jfunction) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  try {
    return getMapPtrFromByteBuffer(env, self)
        ->replaceOne(key.get(), multimap::jni::JavaFunction(env, jfunction));
  } catch (std::exception& error) {
    multimap::jni::propagateOrRethrow(env, error);
    return false;
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    replaceAll
 * Signature: (Ljava/nio/ByteBuffer;[B[B[B)I
 */
JNIEXPORT jint JNICALL
Java_io_multimap_Map_00024Native_replaceAll__Ljava_nio_ByteBuffer_2_3B_3B_3B(
    JNIEnv* env, jclass, jobject self, jbyteArray jkey, jbyteArray jold_value,
    jbyteArray jnew_value) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  multimap::jni::BytesRaiiHelper old_value(env, jold_value);
  multimap::jni::BytesRaiiHelper new_value(env, jnew_value);
  return getMapPtrFromByteBuffer(env, self)
      ->replaceAll(key.get(), old_value.get(), new_value.get());
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    replaceAll
 * Signature: (Ljava/nio/ByteBuffer;[BLio/multimap/Callables/Function;)I
 */
JNIEXPORT jint JNICALL
Java_io_multimap_Map_00024Native_replaceAll__Ljava_nio_ByteBuffer_2_3BLio_multimap_Callables_00024Function_2(
    JNIEnv* env, jclass, jobject self, jbyteArray jkey, jobject jfunction) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  try {
    return getMapPtrFromByteBuffer(env, self)
        ->replaceAll(key.get(), multimap::jni::JavaFunction(env, jfunction));
  } catch (std::exception& error) {
    multimap::jni::propagateOrRethrow(env, error);
    return 0;
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    forEachKey
 * Signature: (Ljava/nio/ByteBuffer;Lio/multimap/Callables/Procedure;)V
 */
JNIEXPORT void JNICALL
Java_io_multimap_Map_00024Native_forEachKey(JNIEnv* env, jclass, jobject self,
                                            jobject jprocedure) {
  try {
    getMapPtrFromByteBuffer(env, self)
        ->forEachKey(multimap::jni::JavaProcedure(env, jprocedure));
  } catch (std::exception& error) {
    multimap::jni::propagateOrRethrow(env, error);
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    forEachValue
 * Signature: (Ljava/nio/ByteBuffer;[BLio/multimap/Callables/Procedure;)V
 */
JNIEXPORT void JNICALL
Java_io_multimap_Map_00024Native_forEachValue(JNIEnv* env, jclass, jobject self,
                                              jbyteArray jkey,
                                              jobject jprocedure) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  try {
    getMapPtrFromByteBuffer(env, self)->forEachValue(
        key.get(), multimap::jni::JavaProcedure(env, jprocedure));
  } catch (std::exception& error) {
    multimap::jni::propagateOrRethrow(env, error);
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    getStats
 * Signature: (Ljava/nio/ByteBuffer;Lio/multimap/Map/Stats;)V
 */
JNIEXPORT void JNICALL
Java_io_multimap_Map_00024Native_getStats(JNIEnv* env, jclass, jobject self,
                                          jobject jstats) {
  const auto cls = env->GetObjectClass(jstats);
  const auto mid =
      env->GetMethodID(cls, "parseFromBuffer", "(Ljava/nio/ByteBuffer;)V");
  mt::Check::notNull(mid, "GetMethodID() failed");
  auto stats = getMapPtrFromByteBuffer(env, self)->getTotalStats();
  env->CallVoidMethod(jstats, mid,
                      env->NewDirectByteBuffer(&stats, sizeof stats));
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    isReadOnly
 * Signature: (Ljava/nio/ByteBuffer;)Z
 */
JNIEXPORT jboolean JNICALL
Java_io_multimap_Map_00024Native_isReadOnly(JNIEnv* env, jclass, jobject self) {
  return getMapPtrFromByteBuffer(env, self)->isReadOnly();
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    close
 * Signature: (Ljava/nio/ByteBuffer;)V
 */
JNIEXPORT void JNICALL
Java_io_multimap_Map_00024Native_close(JNIEnv* env, jclass, jobject self) {
  delete getMapPtrFromByteBuffer(env, self);
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    stats
 * Signature: (Ljava/lang/String;Lio/multimap/Map/Stats;)V
 */
JNIEXPORT void JNICALL
Java_io_multimap_Map_00024Native_stats(JNIEnv* env, jclass, jstring jdirectory,
                                       jobject jstats) {
  const auto directory = multimap::jni::makeString(env, jdirectory);
  try {
    const auto cls = env->GetObjectClass(jstats);
    const auto mid =
        env->GetMethodID(cls, "parseFromBuffer", "(Ljava/nio/ByteBuffer;)V");
    mt::Check::notNull(mid, "GetMethodID() failed");
    auto stats = multimap::Map::Stats::total(multimap::Map::stats(directory));
    env->CallVoidMethod(jstats, mid,
                        env->NewDirectByteBuffer(&stats, sizeof stats));
  } catch (std::exception& error) {
    multimap::jni::throwJavaException(env, error.what());
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    importFromBase64
 * Signature: (Ljava/lang/String;Ljava/lang/String;Lio/multimap/Options;)V
 */
JNIEXPORT void JNICALL Java_io_multimap_Map_00024Native_importFromBase64(
    JNIEnv* env, jclass, jstring jdirectory, jstring jinput, jobject joptions) {
  const auto directory = multimap::jni::makeString(env, jdirectory);
  const auto input = multimap::jni::makeString(env, jinput);
  const auto options = multimap::jni::makeMapOptions(env, joptions);
  try {
    multimap::Map::importFromBase64(directory, input, options);
  } catch (std::exception& error) {
    multimap::jni::throwJavaException(env, error.what());
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    exportToBase64
 * Signature: (Ljava/lang/String;Ljava/lang/String;Lio/multimap/Options;)V
 */
JNIEXPORT void JNICALL
Java_io_multimap_Map_00024Native_exportToBase64(JNIEnv* env, jclass,
                                                jstring jdirectory,
                                                jstring joutput,
                                                jobject joptions) {
  const auto directory = multimap::jni::makeString(env, jdirectory);
  const auto output = multimap::jni::makeString(env, joutput);
  const auto options = multimap::jni::makeMapOptions(env, joptions);
  try {
    multimap::Map::exportToBase64(directory, output, options);
  } catch (std::exception& error) {
    multimap::jni::throwJavaException(env, error.what());
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    optimize
 * Signature: (Ljava/lang/String;Ljava/lang/String;Lio/multimap/Options;)V
 */
JNIEXPORT void JNICALL
Java_io_multimap_Map_00024Native_optimize(JNIEnv* env, jclass,
                                          jstring jdirectory, jstring joutput,
                                          jobject joptions) {
  const auto directory = multimap::jni::makeString(env, jdirectory);
  const auto output = multimap::jni::makeString(env, joutput);
  const auto options = multimap::jni::makeMapOptions(env, joptions);
  try {
    multimap::Map::optimize(directory, output, options);
  } catch (std::exception& error) {
    multimap::jni::throwJavaException(env, error.what());
  }
}
