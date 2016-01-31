#!/usr/bin/env bash

# This script generates C/C++ header files for JNI usage.

set -e  # Exit on error
# set -x  # Display commands

BUILD_DIR=classes
PACKAGE=io/multimap
SOURCE_DIR=java
SOURCE_FILES=($SOURCE_DIR/$PACKAGE/Iterator.java \
              $SOURCE_DIR/$PACKAGE/Map.java)

mkdir $BUILD_DIR
javac -sourcepath $SOURCE_DIR -d $BUILD_DIR ${SOURCE_FILES[*]}

PACKAGE=io.multimap
CLASS_FILES=($PACKAGE.Iterator $PACKAGE.Map)
OUTPUT_DIR=cpp/multimap/jni/generated

javah -jni -classpath $BUILD_DIR -d $OUTPUT_DIR ${CLASS_FILES[*]}
rm -rf $BUILD_DIR

shopt -s extglob
rm $OUTPUT_DIR/!(*Native*)

# Fix wrongly generated function names that contain inner class names.
sed -i 's/Callables_Predicate/Callables_00024Predicate/g' $OUTPUT_DIR/*Native.h
sed -i 's/Callables_Procedure/Callables_00024Procedure/g' $OUTPUT_DIR/*Native.h
sed -i 's/Callables_Function/Callables_00024Function/g' $OUTPUT_DIR/*Native.h
sed -i 's/Callables_LessThan/Callables_00024LessThan/g' $OUTPUT_DIR/*Native.h
