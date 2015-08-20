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
    } else if (entry.first == "block-pool-memory") {
      options.block_pool_memory = std::stoul(entry.second);
    } else if (entry.first == "create-if-missing") {
      options.create_if_missing = (entry.second == "true");
    } else if (entry.first == "error-if-exists") {
      options.error_if_exists = (entry.second == "true");
    }else if (entry.first == "verbose") {
      options.verbose = (entry.second == "true");
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
  try {
    const auto directory = multimap::jni::MakeString(env, jdirectory);
    const auto options = MakeOptions(env, joptions);
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
  Cast(env, self)->Put(key.get(), value.get());
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
  auto iter = Cast(env, self)->Get(key.get());
  if (iter.NumValues() != 0) {
    const auto holder = multimap::jni::MakeHolder(std::move(iter));
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
  auto iter = Cast(env, self)->GetMutable(key.get());
  if (iter.NumValues() != 0) {
    const auto holder = multimap::jni::MakeHolder(std::move(iter));
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
  return Cast(env, self)->Contains(key.get());
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
  return Cast(env, self)->Delete(key.get());
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
  return Cast(env, self)->DeleteFirst(key.get(), predicate);
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
  return Cast(env, self)->DeleteFirstEqual(key.get(), value.get());
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
  return Cast(env, self)->DeleteAll(key.get(), predicate);
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
  return Cast(env, self)->DeleteAllEqual(key.get(), value.get());
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
  return Cast(env, self)->ReplaceFirst(key.get(), function);
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
      ->ReplaceFirstEqual(key.get(), old_value.get(), new_value.get());
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
  return Cast(env, self)->ReplaceAll(key.get(), function);
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
      ->ReplaceAllEqual(key.get(), old_value.get(), new_value.get());
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
  Cast(env, self)->ForEachKey(procedure);
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    getProperties
 * Signature: (Ljava/nio/ByteBuffer;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
    Java_io_multimap_Map_00024Native_getProperties(JNIEnv* env, jclass,
                                                   jobject self) {
  const auto properties = Cast(env, self)->GetProperties();
  return env->NewStringUTF(SerializeProperties(properties).c_str());
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    printProperties
 * Signature: (Ljava/nio/ByteBuffer;)V
 */
JNIEXPORT void JNICALL
    Java_io_multimap_Map_00024Native_printProperties(JNIEnv* env, jclass,
                                                     jobject self) {
  Cast(env, self)->PrintProperties();
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    copy
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
    Java_io_multimap_Map_00024Native_copy__Ljava_lang_String_2Ljava_lang_String_2(
        JNIEnv* env, jclass, jstring jfrom, jstring jto) {
  const auto from = multimap::jni::MakeString(env, jfrom);
  const auto to = multimap::jni::MakeString(env, jto);
  multimap::Copy(from, to);
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    copy
 * Signature: (Ljava/lang/String;Ljava/lang/String;S)V
 */
JNIEXPORT void JNICALL
    Java_io_multimap_Map_00024Native_copy__Ljava_lang_String_2Ljava_lang_String_2S(
        JNIEnv* env, jclass, jstring jfrom, jstring jto,
        jshort new_block_size) {
  const auto from = multimap::jni::MakeString(env, jfrom);
  const auto to = multimap::jni::MakeString(env, jto);
  multimap::Copy(from, to, new_block_size);
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    copy
 * Signature:
 * (Ljava/lang/String;Ljava/lang/String;Lio/multimap/Callables/LessThan;)V
 */
JNIEXPORT void JNICALL
    Java_io_multimap_Map_00024Native_copy__Ljava_lang_String_2Ljava_lang_String_2Lio_multimap_Callables_LessThan_2(
        JNIEnv* env, jclass, jstring jfrom, jstring jto, jobject jless_than) {
  const auto from = multimap::jni::MakeString(env, jfrom);
  const auto to = multimap::jni::MakeString(env, jto);
  const auto less = multimap::jni::MakeCompare(env, jless_than);
  multimap::Copy(from, to, less);
}

/*
 * Class:     io_multimap_Map_Native
 * Method:    copy
 * Signature:
 * (Ljava/lang/String;Ljava/lang/String;Lio/multimap/Callables/LessThan;S)V
 */
JNIEXPORT void JNICALL
    Java_io_multimap_Map_00024Native_copy__Ljava_lang_String_2Ljava_lang_String_2Lio_multimap_Callables_LessThan_2S(
        JNIEnv* env, jclass, jstring jfrom, jstring jto, jobject jless_than,
        jshort new_block_size) {
  const auto from = multimap::jni::MakeString(env, jfrom);
  const auto to = multimap::jni::MakeString(env, jto);
  const auto less = multimap::jni::MakeCompare(env, jless_than);
  multimap::Copy(from, to, less, new_block_size);
}
