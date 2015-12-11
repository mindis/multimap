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

#include "multimap/jni/generated/io_multimap_Map_Limits_Native.h"

#include "multimap/Map.hpp"

/*
 * Class:     io_multimap_Map_Limits_Native
 * Method:    maxKeySize
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
    Java_io_multimap_Map_00024Limits_00024Native_maxKeySize(JNIEnv*, jclass) {
  return multimap::Map::Limits::maxKeySize();
}

/*
 * Class:     io_multimap_Map_Limits_Native
 * Method:    maxValueSize
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
    Java_io_multimap_Map_00024Limits_00024Native_maxValueSize(JNIEnv*, jclass) {
  return multimap::Map::Limits::maxValueSize();
}
