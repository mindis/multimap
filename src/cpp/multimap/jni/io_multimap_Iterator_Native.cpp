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

#include "multimap/jni/generated/io_multimap_Iterator_Native.h"

#include "multimap/jni/common.hpp"
#include "multimap/Map.hpp"

namespace {

typedef multimap::jni::Owner<multimap::Map::Iterator> IteratorOwner;

inline IteratorOwner* toIteratorOwner(JNIEnv* env, jobject self) {
  MT_REQUIRE_NOT_NULL(self);
  return static_cast<IteratorOwner*>(env->GetDirectBufferAddress(self));
}

} // namespace

/*
 * Class:     io_multimap_Iterator_Native
 * Method:    available
 * Signature: (Ljava/nio/ByteBuffer;)J
 */
JNIEXPORT jlong JNICALL
Java_io_multimap_Iterator_00024Native_available(JNIEnv* env, jclass,
                                                jobject self) {
  return toIteratorOwner(env, self)->get().available();
}

/*
 * Class:     io_multimap_Iterator_Native
 * Method:    hasNext
 * Signature: (Ljava/nio/ByteBuffer;)Z
 */
JNIEXPORT jboolean JNICALL
Java_io_multimap_Iterator_00024Native_hasNext(JNIEnv* env, jclass,
                                              jobject self) {
  return toIteratorOwner(env, self)->get().hasNext();
}

/*
 * Class:     io_multimap_Iterator_Native
 * Method:    next
 * Signature: (Ljava/nio/ByteBuffer;)Ljava/nio/ByteBuffer;
 */
JNIEXPORT jobject JNICALL
Java_io_multimap_Iterator_00024Native_next(JNIEnv* env, jclass, jobject self) {
  return multimap::jni::newDirectByteBuffer(
      env, toIteratorOwner(env, self)->get().next());
}

/*
 * Class:     io_multimap_Iterator_Native
 * Method:    peekNext
 * Signature: (Ljava/nio/ByteBuffer;)Ljava/nio/ByteBuffer;
 */
JNIEXPORT jobject JNICALL
Java_io_multimap_Iterator_00024Native_peekNext(JNIEnv* env, jclass,
                                               jobject self) {
  return multimap::jni::newDirectByteBuffer(
      env, toIteratorOwner(env, self)->get().peekNext());
}

/*
 * Class:     io_multimap_Iterator_Native
 * Method:    close
 * Signature: (Ljava/nio/ByteBuffer;)V
 */
JNIEXPORT void JNICALL
Java_io_multimap_Iterator_00024Native_close(JNIEnv* env, jclass, jobject self) {
  delete toIteratorOwner(env, self);
}
