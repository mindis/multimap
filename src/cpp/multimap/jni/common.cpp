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

#include "multimap/jni/common.hpp"

#include <stdexcept>

namespace multimap {
namespace jni {

void propagateOrRethrow(JNIEnv* env, const std::exception& error) {
  if (env->ExceptionOccurred()) {
    // The Java code called before has thrown an exception.
    // Not calling env->ExceptionClear() will propagate it to the JVM.
  } else {
    throwJavaException(env, error.what());
  }
}

void throwJavaException(JNIEnv* env, const char* message) {
  const auto cls = env->FindClass("java/lang/Exception");
  mt::Check::notNull(cls, "FindClass() failed");
  env->ThrowNew(cls, message);
}

std::string makeString(JNIEnv* env, jstring string) {
  const auto chars = env->GetStringUTFChars(string, nullptr);
  mt::Check::notNull(chars, "GetStringUTFChars() failed");
  std::string result = chars;
  env->ReleaseStringUTFChars(string, chars);
  return result;
}

Options makeOptions(JNIEnv* env, jobject options) {
  MT_REQUIRE_NOT_NULL(options);
  const auto cls = env->GetObjectClass(options);
  mt::Check::notNull(cls, "GetObjectClass(options) failed");

  Options opts;
  const auto fid_numPartitions = env->GetFieldID(cls, "numPartitions", "I");
  mt::Check::notNull(fid_numPartitions, "GetFieldID(numPartitions) failed");
  opts.num_partitions = env->GetIntField(options, fid_numPartitions);

  const auto fid_blockSize = env->GetFieldID(cls, "blockSize", "I");
  mt::Check::notNull(fid_blockSize, "GetFieldID(blockSize) failed");
  opts.block_size = env->GetIntField(options, fid_blockSize);

  const auto fid_createIfMissing = env->GetFieldID(cls, "createIfMissing", "Z");
  mt::Check::notNull(fid_createIfMissing, "GetFieldID(createIfMissing) failed");
  opts.create_if_missing = env->GetBooleanField(options, fid_createIfMissing);

  const auto fid_errorIfExists = env->GetFieldID(cls, "errorIfExists", "Z");
  mt::Check::notNull(fid_errorIfExists, "GetFieldID(errorIfExists) failed");
  opts.error_if_exists = env->GetBooleanField(options, fid_errorIfExists);

  const auto fid_readonly = env->GetFieldID(cls, "readonly", "Z");
  mt::Check::notNull(fid_readonly, "GetFieldID(readonly) failed");
  opts.readonly = env->GetBooleanField(options, fid_readonly);

  const auto fid_quiet = env->GetFieldID(cls, "quiet", "Z");
  mt::Check::notNull(fid_quiet, "GetFieldID(quiet) failed");
  opts.quiet = env->GetBooleanField(options, fid_quiet);

  const auto fid_lessThan =
      env->GetFieldID(cls, "lessThan", "Lio/multimap/Callables$LessThan;");
  mt::Check::notNull(fid_lessThan, "GetFieldID(lessThan) failed");
  const auto less_than = env->GetObjectField(options, fid_lessThan);
  if (less_than) {
    opts.compare = JavaCompare(env, less_than);
  }

  return opts;
}

} // namespace jni
} // namespace multimap
