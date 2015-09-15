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

#include "multimap/jni/io_multimap_Map_MutableListIter_Native.h"
#include "multimap/jni/common.hpp"

namespace {

typedef multimap::jni::Holder<multimap::Map::ListIter> ListIterHolder;

inline ListIterHolder* Cast(JNIEnv* env, jobject self) {
  assert(self != nullptr);
  return static_cast<ListIterHolder*>(env->GetDirectBufferAddress(self));
}

}  // namespace

/*
 * Class:     io_multimap_Map_MutableListIter_Native
 * Method:    numValues
 * Signature: (Ljava/nio/ByteBuffer;)J
 */
JNIEXPORT jlong JNICALL
    Java_io_multimap_Map_00024MutableListIter_00024Native_numValues(
        JNIEnv* env, jclass, jobject self) {
  return Cast(env, self)->get().num_values();
}

/*
 * Class:     io_multimap_Map_MutableListIter_Native
 * Method:    seekToFirst
 * Signature: (Ljava/nio/ByteBuffer;)V
 */
JNIEXPORT void JNICALL
    Java_io_multimap_Map_00024MutableListIter_00024Native_seekToFirst(
        JNIEnv* env, jclass, jobject self) {
  Cast(env, self)->get().seekToFirst();
}

/*
 * Class:     io_multimap_Map_MutableListIter_Native
 * Method:    seekTo
 * Signature: (Ljava/nio/ByteBuffer;[B)V
 */
JNIEXPORT void JNICALL
    Java_io_multimap_Map_00024MutableListIter_00024Native_seekTo__Ljava_nio_ByteBuffer_2_3B(
        JNIEnv* env, jclass, jobject self, jbyteArray jtarget) {
  multimap::jni::BytesRaiiHelper target(env, jtarget);
  Cast(env, self)->get().seekTo(target.get());
}

/*
 * Class:     io_multimap_Map_MutableListIter_Native
 * Method:    seekTo
 * Signature: (Ljava/nio/ByteBuffer;Lio/multimap/Callables/Predicate;)V
 */
JNIEXPORT void JNICALL
    Java_io_multimap_Map_00024MutableListIter_00024Native_seekTo__Ljava_nio_ByteBuffer_2Lio_multimap_Callables_Predicate_2(
        JNIEnv* env, jclass, jobject self, jobject jpredicate) {
  const auto predicate = multimap::jni::MakePredicate(env, jpredicate);
  Cast(env, self)->get().seekTo(predicate);
}

/*
 * Class:     io_multimap_Map_MutableListIter_Native
 * Method:    hasValue
 * Signature: (Ljava/nio/ByteBuffer;)Z
 */
JNIEXPORT jboolean JNICALL
    Java_io_multimap_Map_00024MutableListIter_00024Native_hasValue(
        JNIEnv* env, jclass, jobject self) {
  return Cast(env, self)->get().hasValue();
}

/*
 * Class:     io_multimap_Map_MutableListIter_Native
 * Method:    getValue
 * Signature: (Ljava/nio/ByteBuffer;)Ljava/nio/ByteBuffer;
 */
JNIEXPORT jobject JNICALL
    Java_io_multimap_Map_00024MutableListIter_00024Native_getValue(
        JNIEnv* env, jclass, jobject self) {
  const auto val = Cast(env, self)->get().getValue();
  return env->NewDirectByteBuffer(const_cast<char*>(val.data()), val.size());
}

/*
 * Class:     io_multimap_Map_MutableListIter_Native
 * Method:    deleteValue
 * Signature: (Ljava/nio/ByteBuffer;)V
 */
JNIEXPORT void JNICALL
    Java_io_multimap_Map_00024MutableListIter_00024Native_deleteValue(
        JNIEnv* env, jclass, jobject self) {
  Cast(env, self)->get().markAsDeleted();
}

/*
 * Class:     io_multimap_Map_MutableListIter_Native
 * Method:    next
 * Signature: (Ljava/nio/ByteBuffer;)V
 */
JNIEXPORT void JNICALL
    Java_io_multimap_Map_00024MutableListIter_00024Native_next(
        JNIEnv* env, jclass, jobject self) {
  Cast(env, self)->get().next();
}

/*
 * Class:     io_multimap_Map_MutableListIter_Native
 * Method:    close
 * Signature: (Ljava/nio/ByteBuffer;)V
 */
JNIEXPORT void JNICALL
    Java_io_multimap_Map_00024MutableListIter_00024Native_close(
        JNIEnv* env, jclass, jobject self) {
  delete Cast(env, self);
}
