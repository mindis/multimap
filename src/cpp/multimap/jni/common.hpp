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

#ifndef MULTIMAP_JNI_COMMON_HPP_INCLUDED
#define MULTIMAP_JNI_COMMON_HPP_INCLUDED

#include <jni.h>
#include <exception>
#include "multimap/thirdparty/mt/mt.hpp"
#include "multimap/Bytes.hpp"
#include "multimap/Options.hpp"

namespace multimap {
namespace jni {

struct BytesRaiiHelper : mt::Resource {
  BytesRaiiHelper(JNIEnv* env, jbyteArray array)
      : env_(env),
        array_(array),
        bytes_(env->GetByteArrayElements(array, nullptr),
               env->GetArrayLength(array)) {}

  BytesRaiiHelper(JNIEnv* env, jobject array)
      : BytesRaiiHelper(env, static_cast<jbyteArray>(array)) {}

  ~BytesRaiiHelper() {
    auto data = reinterpret_cast<jbyte*>(const_cast<char*>(bytes_.data()));
    env_->ReleaseByteArrayElements(array_, data, JNI_ABORT);
  }

  const Bytes& get() const { return bytes_; }

private:
  JNIEnv* env_;
  const jbyteArray array_;
  const Bytes bytes_;
};

template <typename T> struct Owner : mt::Resource {
  Owner(T&& value) : value_(std::move(value)) {}

  T& get() { return value_; }

private:
  T value_;
};

template <typename T> Owner<T>* newOwner(T&& value) {
  return new Owner<T>(std::move(value));
}

inline jobject newDirectByteBuffer(JNIEnv* env, const Bytes& bytes) {
  return env->NewDirectByteBuffer(const_cast<char*>(bytes.data()),
                                  bytes.size());
}

template <typename T> jobject toDirectByteBuffer(JNIEnv* env, T* ptr) {
  MT_REQUIRE_NOT_NULL(ptr);
  return env->NewDirectByteBuffer(ptr, sizeof ptr);
}

template <typename T> T* fromDirectByteBuffer(JNIEnv* env, jobject jbuf) {
  MT_REQUIRE_NOT_NULL(jbuf);
  return static_cast<T*>(env->GetDirectBufferAddress(jbuf));
}

class JavaCallable {
public:
  JavaCallable(JNIEnv* env, jobject obj, const char* signature)
      : env_(env), obj_(obj) {
    const auto cls = env->GetObjectClass(obj);
    mid_ = env->GetMethodID(cls, "call", signature);
    mt::Check::notNull(mid_, "GetMethodID() failed");
  }

protected:
  JNIEnv* env_;
  jobject obj_;
  jmethodID mid_;
};

class JavaCompare : public JavaCallable {
public:
  typedef JavaCallable Base;

  JavaCompare(JNIEnv* env, jobject obj)
      : Base(env, obj, "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)Z") {}

  bool operator()(const multimap::Bytes& lhs,
                  const multimap::Bytes& rhs) const {
    // Note: java.nio.ByteBuffer cannot wrap a pointer to const void.
    // However, on Java side we will call ByteBuffer.asReadOnlyBuffer().
    const auto result =
        env_->CallBooleanMethod(obj_, mid_, newDirectByteBuffer(env_, lhs),
                                newDirectByteBuffer(env_, rhs));
    if (env_->ExceptionOccurred()) {
      throw std::runtime_error("Exception in comparator passed via JNI");
      // This exception is to escape from the for-each loop.
      // Since env->ExceptionClear() is not called the actual exception
      // is passed to the Java exception-handling process of the Java client.
    }
    return result;
  }
};

class JavaFunction : public JavaCallable {
public:
  typedef JavaCallable Base;

  JavaFunction(JNIEnv* env, jobject obj)
      : Base(env, obj, "(Ljava/nio/ByteBuffer;)[B") {}

  std::string operator()(const multimap::Bytes& bytes) const {
    // Note: java.nio.ByteBuffer cannot wrap a pointer to const void.
    // However, on Java side we will call ByteBuffer.asReadOnlyBuffer().
    const auto result =
        env_->CallObjectMethod(obj_, mid_, newDirectByteBuffer(env_, bytes));
    if (env_->ExceptionOccurred()) {
      throw std::runtime_error("Exception in function passed via JNI");
      // This exception is to escape from the for-each loop.
      // Since env->ExceptionClear() is not called the actual exception
      // is passed to the Java exception-handling process of the Java client.
    }
    // result is a jbyteArray that is copied into a std::string.
    return (result != nullptr) ? BytesRaiiHelper(env_, result).get().toString()
                               : std::string();
  }
};

class JavaPredicate : public JavaCallable {
public:
  typedef JavaCallable Base;

  JavaPredicate(JNIEnv* env, jobject obj)
      : Base(env, obj, "(Ljava/nio/ByteBuffer;)Z") {}

  bool operator()(const multimap::Bytes& bytes) const {
    // Note: java.nio.ByteBuffer cannot wrap a pointer to const void.
    // However, on Java side we will call ByteBuffer.asReadOnlyBuffer().
    const auto result =
        env_->CallBooleanMethod(obj_, mid_, newDirectByteBuffer(env_, bytes));
    if (env_->ExceptionOccurred()) {
      throw std::runtime_error("Exception in predicate passed via JNI");
      // This exception is to escape from the for-each loop.
      // Since env->ExceptionClear() is not called the actual exception
      // is passed to the Java exception-handling process of the Java client.
    }
    return result;
  }
};

class JavaProcedure : public JavaCallable {
public:
  typedef JavaCallable Base;

  JavaProcedure(JNIEnv* env, jobject obj)
      : Base(env, obj, "(Ljava/nio/ByteBuffer;)V") {}

  void operator()(const multimap::Bytes& bytes) const {
    // Note: java.nio.ByteBuffer cannot wrap a pointer to const void.
    // However, on Java side we will call ByteBuffer.asReadOnlyBuffer().
    env_->CallVoidMethod(obj_, mid_, newDirectByteBuffer(env_, bytes));
    if (env_->ExceptionOccurred()) {
      throw std::runtime_error("Exception in procedure passed via JNI");
      // This exception is to escape from the for-each loop.
      // Since env->ExceptionClear() is not called the actual exception
      // is passed to the Java exception-handling process of the Java client.
    }
  }
};

void propagateOrRethrow(JNIEnv* env, const std::exception& error);

void throwJavaException(JNIEnv* env, const char* message);

std::string makeString(JNIEnv* env, jstring string);

Options makeOptions(JNIEnv* env, jobject options);

} // namespace jni
} // namespace multimap

#endif // MULTIMAP_JNI_COMMON_HPP_INCLUDED
