#!/bin/bash

# Generates C/C++ header files from Java class files with native method declaration.

CLASSPATH=../bin
CLASSES=io.multimap.Map
OUTPUT_DIR=../src/cpp/multimap/jni/generated

javah -d $OUTPUT_DIR -jni -classpath $CLASSPATH $CLASSES
shopt -s extglob
rm $OUTPUT_DIR/!(*Native*)
