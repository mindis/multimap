// This file is part of the Multimap library.  http://multimap.io
//
// Copyright (C) 2015  Martin Trenkmann
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
#include "multimap/Map.hpp"

// Bug in javah -jni (OpenJDK 1.7.0_79, Debian 7.8)
//
// For io.multimap.Map.Native.forEachValue(ByteBuffer, Callables.Procedure)
// the following signature is generated:
//
// (Ljava/nio/ByteBuffer;[BLio/multimap/Callables/Procedure;)V
// Java_io_multimap_Map_00024Native_forEachValue__Ljava_nio_ByteBuffer_2_3BLio_multimap_Callables_Procedure_2
//
// However, Callables.Procedure is an inner class similar to Map.Native, and
// should therefore generate the following signature:
//
// (Ljava/nio/ByteBuffer;[BLio/multimap/Callables$Procedure;)V
// Java_io_multimap_Map_00024Native_forEachValue__Ljava_nio_ByteBuffer_2_3BLio_multimap_Callables_00024Procedure_2
//
// The fix has to be applied manually at the moment for every such case.

namespace {

inline multimap::Map* toMap(JNIEnv* env, jobject jbuf) {
  return multimap::jni::fromDirectByteBuffer<multimap::Map>(env, jbuf);
}

} // namespace

/*
 * Class:     io_multimap_Map_Native
 * Method:    newMap
 * Signature: (Ljava/lang/String;Lio/multimap/Options;)Ljava/nio/ByteBuffer;
 */
JNIEXPORT jobject JNICALL
    Java_io_multimap_Map_00024Native_newMap(JNIEnv* env, jclass,
                                            jstring jdirectory,
                                            jobject joptions) {
  const auto directory = multimap::jni::toString(env, jdirectory);
  const auto options = multimap::jni::toOptions(env, joptions);
  try {
    const auto map = new multimap::Map(directory, options);
    return multimap::jni::toDirectByteBuffer(env, map);
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
    toMap(env, self)->put(key.get(), value.get());
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
  auto iter = toMap(env, self)->get(key.get());
  if (iter.hasNext()) {
    return multimap::jni::toDirectByteBuffer(
        env, multimap::jni::newOwner(std::move(iter)));
  }
  return nullptr;
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    getMutable
 * Signature: (Ljava/nio/ByteBuffer;[B)Ljava/nio/ByteBuffer;
 */
JNIEXPORT jobject JNICALL
    Java_io_multimap_Map_00024Native_getMutable(JNIEnv* env, jclass,
                                                jobject self, jbyteArray jkey) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  auto iter = toMap(env, self)->getMutable(key.get());
  if (iter.hasNext()) {
    return multimap::jni::toDirectByteBuffer(
        env, multimap::jni::newOwner(std::move(iter)));
  }
  return nullptr;
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    remove
 * Signature: (Ljava/nio/ByteBuffer;[B)J
 */
JNIEXPORT jlong JNICALL
    Java_io_multimap_Map_00024Native_remove(JNIEnv* env, jclass, jobject self,
                                            jbyteArray jkey) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  return toMap(env, self)->remove(key.get());
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    removeAll
 * Signature: (Ljava/nio/ByteBuffer;[BLio/multimap/Callables/Predicate;)J
 */
JNIEXPORT jlong JNICALL
    Java_io_multimap_Map_00024Native_removeAll(JNIEnv* env, jclass,
                                               jobject self, jbyteArray jkey,
                                               jobject jpredicate) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  const auto predicate = multimap::jni::toPredicate(env, jpredicate);
  try {
    return toMap(env, self)->removeAll(key.get(), predicate);
  } catch (std::exception& error) {
    multimap::jni::propagateOrRethrow(env, error);
    return 0;
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    removeAllEqual
 * Signature: (Ljava/nio/ByteBuffer;[B[B)J
 */
JNIEXPORT jlong JNICALL Java_io_multimap_Map_00024Native_removeAllEqual(
    JNIEnv* env, jclass, jobject self, jbyteArray jkey, jbyteArray jvalue) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  multimap::jni::BytesRaiiHelper value(env, jvalue);
  return toMap(env, self)->removeAllEqual(key.get(), value.get());
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    removeFirst
 * Signature: (Ljava/nio/ByteBuffer;[BLio/multimap/Callables/Predicate;)Z
 */
JNIEXPORT jboolean JNICALL
    Java_io_multimap_Map_00024Native_removeFirst(JNIEnv* env, jclass,
                                                 jobject self, jbyteArray jkey,
                                                 jobject jpredicate) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  const auto predicate = multimap::jni::toPredicate(env, jpredicate);
  try {
    return toMap(env, self)->removeFirst(key.get(), predicate);
  } catch (std::exception& error) {
    multimap::jni::propagateOrRethrow(env, error);
    return false;
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    removeFirstEqual
 * Signature: (Ljava/nio/ByteBuffer;[B[B)Z
 */
JNIEXPORT jboolean JNICALL Java_io_multimap_Map_00024Native_removeFirstEqual(
    JNIEnv* env, jclass, jobject self, jbyteArray jkey, jbyteArray jvalue) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  multimap::jni::BytesRaiiHelper value(env, jvalue);
  return toMap(env, self)->removeFirstEqual(key.get(), value.get());
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    replaceAll
 * Signature: (Ljava/nio/ByteBuffer;[BLio/multimap/Callables/Function;)J
 */
JNIEXPORT jlong JNICALL
    Java_io_multimap_Map_00024Native_replaceAll(JNIEnv* env, jclass,
                                                jobject self, jbyteArray jkey,
                                                jobject jfunction) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  const auto function = multimap::jni::toFunction(env, jfunction);
  try {
    return toMap(env, self)->replaceAll(key.get(), function);
  } catch (std::exception& error) {
    multimap::jni::propagateOrRethrow(env, error);
    return 0;
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    replaceAllEqual
 * Signature: (Ljava/nio/ByteBuffer;[B[B[B)J
 */
JNIEXPORT jlong JNICALL Java_io_multimap_Map_00024Native_replaceAllEqual(
    JNIEnv* env, jclass, jobject self, jbyteArray jkey, jbyteArray jold_value,
    jbyteArray jnew_value) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  multimap::jni::BytesRaiiHelper old_value(env, jold_value);
  multimap::jni::BytesRaiiHelper new_value(env, jnew_value);
  return toMap(env, self)
      ->replaceAllEqual(key.get(), old_value.get(), new_value.get());
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    replaceFirst
 * Signature: (Ljava/nio/ByteBuffer;[BLio/multimap/Callables/Function;)Z
 */
JNIEXPORT jboolean JNICALL
    Java_io_multimap_Map_00024Native_replaceFirst(JNIEnv* env, jclass,
                                                  jobject self, jbyteArray jkey,
                                                  jobject jfunction) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  const auto function = multimap::jni::toFunction(env, jfunction);
  try {
    return toMap(env, self)->replaceFirst(key.get(), function);
  } catch (std::exception& error) {
    multimap::jni::propagateOrRethrow(env, error);
    return false;
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    replaceFirstEqual
 * Signature: (Ljava/nio/ByteBuffer;[B[B[B)Z
 */
JNIEXPORT jboolean JNICALL Java_io_multimap_Map_00024Native_replaceFirstEqual(
    JNIEnv* env, jclass, jobject self, jbyteArray jkey, jbyteArray jold_value,
    jbyteArray jnew_value) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  multimap::jni::BytesRaiiHelper old_value(env, jold_value);
  multimap::jni::BytesRaiiHelper new_value(env, jnew_value);
  return toMap(env, self)
      ->replaceFirstEqual(key.get(), old_value.get(), new_value.get());
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    forEachKey
 * Signature: (Ljava/nio/ByteBuffer;Lio/multimap/Callables/Procedure;)V
 */
JNIEXPORT void JNICALL
    Java_io_multimap_Map_00024Native_forEachKey(JNIEnv* env, jclass,
                                                jobject self,
                                                jobject jprocedure) {
  const auto procedure = multimap::jni::toProcedure(env, jprocedure);
  try {
    toMap(env, self)->forEachKey(procedure);
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
    Java_io_multimap_Map_00024Native_forEachValue__Ljava_nio_ByteBuffer_2_3BLio_multimap_Callables_Procedure_2(
        JNIEnv* env, jclass, jobject self, jbyteArray jkey,
        jobject jprocedure) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  const auto procedure = multimap::jni::toProcedure(env, jprocedure);
  try {
    toMap(env, self)->forEachValue(key.get(), procedure);
  } catch (std::exception& error) {
    multimap::jni::propagateOrRethrow(env, error);
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    forEachValue
 * Signature: (Ljava/nio/ByteBuffer;[BLio/multimap/Callables/Predicate;)V
 */
JNIEXPORT void JNICALL
    Java_io_multimap_Map_00024Native_forEachValue__Ljava_nio_ByteBuffer_2_3BLio_multimap_Callables_Predicate_2(
        JNIEnv* env, jclass, jobject self, jbyteArray jkey,
        jobject jpredicate) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  const auto predicate = multimap::jni::toPredicate(env, jpredicate);
  try {
    toMap(env, self)->forEachValue(key.get(), predicate);
  } catch (std::exception& error) {
    multimap::jni::propagateOrRethrow(env, error);
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    isReadOnly
 * Signature: (Ljava/nio/ByteBuffer;)Z
 */
JNIEXPORT jboolean JNICALL
    Java_io_multimap_Map_00024Native_isReadOnly(JNIEnv* env, jclass,
                                                jobject self) {
  return toMap(env, self)->isReadOnly();
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    close
 * Signature: (Ljava/nio/ByteBuffer;)V
 */
JNIEXPORT void JNICALL
    Java_io_multimap_Map_00024Native_close(JNIEnv* env, jclass, jobject self) {
  delete toMap(env, self);
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    importFromBase64
 * Signature: (Ljava/lang/String;Ljava/lang/String;Lio/multimap/Options;)V
 */
JNIEXPORT void JNICALL Java_io_multimap_Map_00024Native_importFromBase64(
    JNIEnv* env, jclass, jstring jdirectory, jstring jinput, jobject joptions) {
  const auto directory = multimap::jni::toString(env, jdirectory);
  const auto input = multimap::jni::toString(env, jinput);
  const auto options = multimap::jni::toOptions(env, joptions);
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
  const auto directory = multimap::jni::toString(env, jdirectory);
  const auto output = multimap::jni::toString(env, joutput);
  const auto options = multimap::jni::toOptions(env, joptions);
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
                                              jstring jsource, jstring jtarget,
                                              jobject joptions) {
  const auto source = multimap::jni::toString(env, jsource);
  const auto target = multimap::jni::toString(env, jtarget);
  const auto options = multimap::jni::toOptions(env, joptions);
  try {
    multimap::Map::optimize(source, target, options);
  } catch (std::exception& error) {
    multimap::jni::throwJavaException(env, error.what());
  }
}
