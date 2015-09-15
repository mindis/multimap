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

#include "multimap/jni/io_multimap_Map_Native.h"

#include <sstream>
#include "multimap/jni/common.hpp"

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

inline multimap::Map* Cast(JNIEnv* env, jobject self) {
  assert(self != nullptr);
  return static_cast<multimap::Map*>(env->GetDirectBufferAddress(self));
}

typedef std::map<std::string, std::string> Properties;

Properties ParseProperties(const std::string& input) {
  Properties props;
  std::string line;
  std::istringstream iss(input);
  while (std::getline(iss, line)) {
    const auto pos = line.find('=');
    if (pos == std::string::npos) {
      std::cerr << "WARNING Properties string contains malformed line: " << line
                << '\n';
      continue;
    }
    const auto key = line.substr(0, pos);
    const auto value = line.substr(pos + 1);
    props[key] = value;
  }
  return props;
}

std::string SerializeProperties(const Properties& properties) {
  std::string result;
  for (const auto& entry : properties) {
    result.append(entry.first);
    result.push_back('=');
    result.append(entry.second);
    result.push_back('\n');
  }
  return result;
}

multimap::Options MakeOptions(JNIEnv* env, jstring joptions) {
  multimap::Options options;
  const auto str = multimap::jni::MakeString(env, joptions);
  for (const auto& entry : ParseProperties(str)) {
    if (entry.first == "block-size") {
      options.block_size = std::stoul(entry.second);
    } else if (entry.first == "create-if-missing") {
      options.create_if_missing = (entry.second == "true");
    } else if (entry.first == "error-if-exists") {
      options.error_if_exists = (entry.second == "true");
    } else if (entry.first == "write-only-mode") {
      options.write_only_mode = (entry.second == "true");
    } else {
      std::cerr << "WARNING Unknown option: " << entry.first << '\n';
    }
  }
  return options;
}

}  // namespace

/*
 * Class:     io_multimap_Map_Native
 * Method:    open
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/nio/ByteBuffer;
 */
JNIEXPORT jobject JNICALL
    Java_io_multimap_Map_00024Native_open(JNIEnv* env, jclass,
                                          jstring jdirectory,
                                          jstring joptions) {
  const auto directory = multimap::jni::MakeString(env, jdirectory);
  const auto options = MakeOptions(env, joptions);
  try {
    const auto map = new multimap::Map(directory, options);
    return env->NewDirectByteBuffer(map, sizeof map);
  } catch (std::exception& error) {
    multimap::jni::ThrowException(env, error.what());
  }
  return nullptr;
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    close
 * Signature: (Ljava/nio/ByteBuffer;)V
 */
JNIEXPORT void JNICALL
    Java_io_multimap_Map_00024Native_close(JNIEnv* env, jclass, jobject self) {
  delete Cast(env, self);
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
    Cast(env, self)->put(key.get(), value.get());
  } catch (std::exception& error) {
    multimap::jni::ThrowException(env, error.what());
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
  auto iter = Cast(env, self)->get(key.get());
  if (iter.num_values() != 0) {
    const auto holder = multimap::jni::NewHolder(std::move(iter));
    return env->NewDirectByteBuffer(holder, sizeof holder);
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
  auto iter = Cast(env, self)->getMutable(key.get());
  if (iter.num_values() != 0) {
    const auto holder = multimap::jni::NewHolder(std::move(iter));
    return env->NewDirectByteBuffer(holder, sizeof holder);
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
  return Cast(env, self)->contains(key.get());
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    delete
 * Signature: (Ljava/nio/ByteBuffer;[B)J
 */
JNIEXPORT jlong JNICALL
    Java_io_multimap_Map_00024Native_delete(JNIEnv* env, jclass, jobject self,
                                            jbyteArray jkey) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  return Cast(env, self)->remove(key.get());
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    deleteAll
 * Signature: (Ljava/nio/ByteBuffer;[BLio/multimap/Callables/Predicate;)J
 */
JNIEXPORT jlong JNICALL
    Java_io_multimap_Map_00024Native_deleteAll(JNIEnv* env, jclass,
                                               jobject self, jbyteArray jkey,
                                               jobject jpredicate) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  const auto predicate = multimap::jni::MakePredicate(env, jpredicate);
  try {
    return Cast(env, self)->removeAll(key.get(), predicate);
  } catch (std::exception& error) {
    if (env->ExceptionOccurred()) {
      // The Java predicate has thrown an exception.
      // Not calling env->ExceptionClear() will forward it to the JVM.
    } else {
      multimap::jni::ThrowException(env, error.what());
      // The C++ code has thrown an exception that is rethrown to the JVM.
    }
    return 0;
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    deleteAllEqual
 * Signature: (Ljava/nio/ByteBuffer;[B[B)J
 */
JNIEXPORT jlong JNICALL Java_io_multimap_Map_00024Native_deleteAllEqual(
    JNIEnv* env, jclass, jobject self, jbyteArray jkey, jbyteArray jvalue) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  multimap::jni::BytesRaiiHelper value(env, jvalue);
  return Cast(env, self)->removeAllEqual(key.get(), value.get());
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    deleteFirst
 * Signature: (Ljava/nio/ByteBuffer;[BLio/multimap/Callables/Predicate;)Z
 */
JNIEXPORT jboolean JNICALL
    Java_io_multimap_Map_00024Native_deleteFirst(JNIEnv* env, jclass,
                                                 jobject self, jbyteArray jkey,
                                                 jobject jpredicate) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  const auto predicate = multimap::jni::MakePredicate(env, jpredicate);
  try {
    return Cast(env, self)->removeFirst(key.get(), predicate);
  } catch (std::exception& error) {
    if (env->ExceptionOccurred()) {
      // The Java predicate has thrown an exception.
      // Not calling env->ExceptionClear() will forward it to the JVM.
    } else {
      multimap::jni::ThrowException(env, error.what());
      // The C++ code has thrown an exception that is rethrown to the JVM.
    }
    return false;
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    deleteFirstEqual
 * Signature: (Ljava/nio/ByteBuffer;[B[B)Z
 */
JNIEXPORT jboolean JNICALL Java_io_multimap_Map_00024Native_deleteFirstEqual(
    JNIEnv* env, jclass, jobject self, jbyteArray jkey, jbyteArray jvalue) {
  multimap::jni::BytesRaiiHelper key(env, jkey);
  multimap::jni::BytesRaiiHelper value(env, jvalue);
  return Cast(env, self)->removeFirstEqual(key.get(), value.get());
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
  const auto function = multimap::jni::MakeFunction(env, jfunction);
  try {
    return Cast(env, self)->replaceAll(key.get(), function);
  } catch (std::exception& error) {
    if (env->ExceptionOccurred()) {
      // The Java function has thrown an exception.
      // Not calling env->ExceptionClear() will forward it to the JVM.
    } else {
      multimap::jni::ThrowException(env, error.what());
      // The C++ code has thrown an exception that is rethrown to the JVM.
    }
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
  return Cast(env, self)
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
  const auto function = multimap::jni::MakeFunction(env, jfunction);
  try {
    return Cast(env, self)->replaceFirst(key.get(), function);
  } catch (std::exception& error) {
    if (env->ExceptionOccurred()) {
      // The Java function has thrown an exception.
      // Not calling env->ExceptionClear() will forward it to the JVM.
    } else {
      multimap::jni::ThrowException(env, error.what());
      // The C++ code has thrown an exception that is rethrown to the JVM.
    }
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
  return Cast(env, self)
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
  const auto procedure = multimap::jni::MakeProcedure(env, jprocedure);
  try {
    Cast(env, self)->forEachKey(procedure);
  } catch (std::exception& error) {
    if (env->ExceptionOccurred()) {
      // The Java procedure has thrown an exception.
      // Not calling env->ExceptionClear() will forward it to the JVM.
    } else {
      multimap::jni::ThrowException(env, error.what());
      // The C++ code has thrown an exception that is rethrown to the JVM.
    }
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    forEachValue
 * Signature: (Ljava/nio/ByteBuffer;[BLio/multimap/Callables$Procedure;)V
 */
JNIEXPORT void JNICALL
    Java_io_multimap_Map_00024Native_forEachValue__Ljava_nio_ByteBuffer_2_3BLio_multimap_Callables_00024Procedure_2(
        JNIEnv* env, jclass, jobject self, jbyteArray jkey,
        jobject jprocedure) {
  const auto procedure = multimap::jni::MakeProcedure(env, jprocedure);
  multimap::jni::BytesRaiiHelper key(env, jkey);
  try {
    Cast(env, self)->forEachValue(key.get(), procedure);
  } catch (std::exception& error) {
    if (env->ExceptionOccurred()) {
      // The Java procedure has thrown an exception.
      // Not calling env->ExceptionClear() will forward it to the JVM.
    } else {
      multimap::jni::ThrowException(env, error.what());
      // The C++ code has thrown an exception that is rethrown to the JVM.
    }
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    forEachValue
 * Signature: (Ljava/nio/ByteBuffer;[BLio/multimap/Callables$Predicate;)V
 */
JNIEXPORT void JNICALL
    Java_io_multimap_Map_00024Native_forEachValue__Ljava_nio_ByteBuffer_2_3BLio_multimap_Callables_00024Predicate_2(
        JNIEnv* env, jclass, jobject self, jbyteArray jkey,
        jobject jpredicate) {
  const auto predicate = multimap::jni::MakePredicate(env, jpredicate);
  multimap::jni::BytesRaiiHelper key(env, jkey);
  try {
    Cast(env, self)->forEachValue(key.get(), predicate);
  } catch (std::exception& error) {
    if (env->ExceptionOccurred()) {
      // The Java predicate has thrown an exception.
      // Not calling env->ExceptionClear() will forward it to the JVM.
    } else {
      multimap::jni::ThrowException(env, error.what());
      // The C++ code has thrown an exception that is rethrown to the JVM.
    }
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    getProperties
 * Signature: (Ljava/nio/ByteBuffer;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
    Java_io_multimap_Map_00024Native_getProperties(JNIEnv* env, jclass,
                                                   jobject self) {
  const auto properties = Cast(env, self)->getProperties();
  return env->NewStringUTF(SerializeProperties(properties).c_str());
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    maxKeySize
 * Signature: (Ljava/nio/ByteBuffer;)I
 */
JNIEXPORT jint JNICALL
    Java_io_multimap_Map_00024Native_maxKeySize(JNIEnv* env, jclass,
                                                jobject self) {
  return Cast(env, self)->max_key_size();
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    maxValueSize
 * Signature: (Ljava/nio/ByteBuffer;)I
 */
JNIEXPORT jint JNICALL
    Java_io_multimap_Map_00024Native_maxValueSize(JNIEnv* env, jclass,
                                                  jobject self) {
  return Cast(env, self)->max_value_size();
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    Optimize
 * Signature:
 * (Ljava/lang/String;Ljava/lang/String;Lio/multimap/Callables/LessThan;I)V
 */
JNIEXPORT void JNICALL
    Java_io_multimap_Map_00024Native_Optimize(JNIEnv* env, jclass,
                                              jstring jfrom, jstring jto,
                                              jobject jless_than,
                                              jint new_block_size) {
  const auto from = multimap::jni::MakeString(env, jfrom);
  const auto to = multimap::jni::MakeString(env, jto);
  const auto compare = (jless_than != nullptr)
                           ? multimap::jni::MakeCompare(env, jless_than)
                           : multimap::Callables::Compare();
  try {
    multimap::optimize(from, to, compare, new_block_size);
  } catch (std::exception& error) {
    multimap::jni::ThrowException(env, error.what());
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    Import
 * Signature: (Ljava/lang/String;Ljava/lang/String;ZI)V
 */
JNIEXPORT void JNICALL
    Java_io_multimap_Map_00024Native_Import(JNIEnv* env, jclass,
                                            jstring jdirectory, jstring jfile,
                                            jboolean create_if_missing,
                                            jint block_size) {
  const auto directory = multimap::jni::MakeString(env, jdirectory);
  const auto file = multimap::jni::MakeString(env, jfile);
  try {
    multimap::importFromBase64(directory, file, create_if_missing, block_size);
  } catch (std::exception& error) {
    multimap::jni::ThrowException(env, error.what());
  }
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    Export
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
    Java_io_multimap_Map_00024Native_Export(JNIEnv* env, jclass,
                                            jstring jdirectory, jstring jfile) {
  const auto directory = multimap::jni::MakeString(env, jdirectory);
  const auto file = multimap::jni::MakeString(env, jfile);
  try {
    multimap::exportToBase64(directory, file);
  } catch (std::exception& error) {
    multimap::jni::ThrowException(env, error.what());
  }
}
