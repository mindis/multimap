#!/usr/bin/env bash

# This script generates C/C++ header files for JNI usage.

set -e  # Exit on error
# set -x  # Display commands

CLASSPATH=bin
CLASSES="io.multimap.Iterator io.multimap.Map"
OUTPUT_DIR=src/cpp/multimap/jni/generated

javah -d $OUTPUT_DIR -jni -classpath $CLASSPATH $CLASSES
shopt -s extglob
rm $OUTPUT_DIR/!(*Native*)

# Fix wrongly generated function names that contain inner class names.
sed -i 's/Callables_Predicate/Callables_00024Predicate/g' $OUTPUT_DIR/*Native.h
sed -i 's/Callables_Procedure/Callables_00024Procedure/g' $OUTPUT_DIR/*Native.h
sed -i 's/Callables_Function/Callables_00024Function/g' $OUTPUT_DIR/*Native.h
sed -i 's/Callables_LessThan/Callables_00024LessThan/g' $OUTPUT_DIR/*Native.h
