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

#ifndef MULTIMAP_JNI_COMMON_HPP
#define MULTIMAP_JNI_COMMON_HPP

#include <jni.h>
#include "multimap/Map.hpp"

namespace multimap {
namespace jni {

struct BytesRaiiHelper {
  BytesRaiiHelper(JNIEnv* env, jbyteArray array)
      : env_(env),
        array_(array),
        bytes_(env->GetByteArrayElements(array, nullptr),
               env->GetArrayLength(array)) {}

  BytesRaiiHelper(JNIEnv* env, jobject array)
      : BytesRaiiHelper(env, static_cast<jbyteArray>(array)) {}

  ~BytesRaiiHelper() {
    auto elements = reinterpret_cast<jbyte*>(const_cast<char*>(bytes_.data()));
    env_->ReleaseByteArrayElements(array_, elements, JNI_ABORT);
  }

  BytesRaiiHelper(BytesRaiiHelper&&) = delete;
  BytesRaiiHelper& operator=(BytesRaiiHelper&&) = delete;

  BytesRaiiHelper(const BytesRaiiHelper&) = delete;
  BytesRaiiHelper& operator=(const BytesRaiiHelper&) = delete;

  const multimap::Bytes& get() const { return bytes_; }

 private:
  JNIEnv* env_;
  const jbyteArray array_;
  const multimap::Bytes bytes_;
};

template <typename T>
struct Holder {
  Holder(T&& element) : element_(std::move(element)) {}

  T& get() { return element_; }

 private:
  T element_;
};

template <typename T>
Holder<T>* NewHolder(T&& element) {
  return new Holder<T>(std::move(element));
}

inline Callables::Procedure MakeProcedure(JNIEnv* env, jobject jprocedure) {
  const auto cls = env->GetObjectClass(jprocedure);
  const auto mid = env->GetMethodID(cls, "call", "(Ljava/nio/ByteBuffer;)V");
  assert(mid != nullptr);
  return [=](const multimap::Bytes& bytes) {
    // Note: java.nio.ByteBuffer cannot wrap a pointer to const void.
    // However, on Java side we will call ByteBuffer.asReadOnlyBuffer().
    env->CallVoidMethod(jprocedure, mid,
                        env->NewDirectByteBuffer(
                            const_cast<char*>(bytes.data()), bytes.size()));
    // TODO Check for excpetions.
  };
}

inline Callables::Predicate MakePredicate(JNIEnv* env, jobject jpredicate) {
  const auto cls = env->GetObjectClass(jpredicate);
  const auto mid = env->GetMethodID(cls, "call", "(Ljava/nio/ByteBuffer;)Z");
  assert(mid != nullptr);
  return [=](const multimap::Bytes& bytes) {
    // Note: java.nio.ByteBuffer cannot wrap a pointer to const void.
    // However, on Java side we will call ByteBuffer.asReadOnlyBuffer().
    return env->CallBooleanMethod(
        jpredicate, mid, env->NewDirectByteBuffer(
                             const_cast<char*>(bytes.data()), bytes.size()));
    // TODO Check for excpetions.
  };
}

inline Callables::Function MakeFunction(JNIEnv* env, jobject jfunction) {
  const auto cls = env->GetObjectClass(jfunction);
  const auto mid = env->GetMethodID(cls, "call", "(Ljava/nio/ByteBuffer;)[B");
  assert(mid != nullptr);
  return [=](const multimap::Bytes& bytes) {
    // Note: java.nio.ByteBuffer cannot wrap a pointer to const void.
    // However, on Java side we will call ByteBuffer.asReadOnlyBuffer().
    const auto result = env->CallObjectMethod(
        jfunction, mid, env->NewDirectByteBuffer(
                            const_cast<char*>(bytes.data()), bytes.size()));
    // result is a jbyteArray that is copied into a std::string.
    return (result != nullptr) ? BytesRaiiHelper(env, result).get().ToString()
                               : std::string();
    // TODO Check for excpetions.
  };
}

inline Callables::Compare MakeCompare(JNIEnv* env, jobject jless_than) {
  const auto cls = env->GetObjectClass(jless_than);
  const auto mid = env->GetMethodID(
      cls, "call", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)Z");
  assert(mid != nullptr);
  return [=](const multimap::Bytes& lhs, const multimap::Bytes& rhs) {
    // Note: java.nio.ByteBuffer cannot wrap a pointer to const void.
    // However, on Java side we will call ByteBuffer.asReadOnlyBuffer().
    return env->CallBooleanMethod(
        jless_than, mid,
        env->NewDirectByteBuffer(const_cast<char*>(lhs.data()), lhs.size()),
        env->NewDirectByteBuffer(const_cast<char*>(rhs.data()), rhs.size()));
    // TODO Check for excpetions.
  };
}

inline std::string MakeString(JNIEnv* env, jstring string) {
  const auto chars = env->GetStringUTFChars(string, nullptr);
  std::string str(chars);
  env->ReleaseStringUTFChars(string, chars);
  return str;
}

inline void ThrowException(JNIEnv* env, const char* message) {
  /* static */ const auto clazz = env->FindClass("java/lang/Exception");
  // static lets the VM crash.
  env->ThrowNew(clazz, message);
}

}  // namespace jni
}  // namespace multimap

#endif  // MULTIMAP_JNI_COMMON_HPP
