#!/usr/bin/env bash

# This script compiles Java source code and packs it into a single JAR file.

set -e  # Exit on error
# set -x  # Display commands

BUILD_DIR=classes
PACKAGE=io/multimap
SOURCE_DIR=src/java
SOURCE_FILES=($SOURCE_DIR/$PACKAGE/Callables.java \
              $SOURCE_DIR/$PACKAGE/Check.java \
              $SOURCE_DIR/$PACKAGE/Iterator.java \
              $SOURCE_DIR/$PACKAGE/Map.java \
              $SOURCE_DIR/$PACKAGE/Options.java)

mkdir $BUILD_DIR
javac -sourcepath $SOURCE_DIR -d $BUILD_DIR ${SOURCE_FILES[*]}

MAJOR_VERSION=$(grep MAJOR_VERSION src/cpp/multimap/version.hpp | cut -d';' -f1 | cut -d' ' -f6)
MINOR_VERSION=$(grep MINOR_VERSION src/cpp/multimap/version.hpp | cut -d';' -f1 | cut -d' ' -f6)
PATCH_VERSION=$(grep PATCH_VERSION src/cpp/multimap/version.hpp | cut -d';' -f1 | cut -d' ' -f6)
JAR_FILE=multimap-$MAJOR_VERSION.$MINOR_VERSION.$PATCH_VERSION.jar

jar cf $JAR_FILE $BUILD_DIR
rm -rf $BUILD_DIR
