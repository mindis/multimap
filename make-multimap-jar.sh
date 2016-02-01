#!/usr/bin/env bash

# This script compiles the Java source code and packs it into a single JAR file.

set -e  # Exit on error
# set -x  # Display commands

JAR_DIR=jar
PACKAGE=io/multimap
SOURCE_DIR=src/java
SOURCE_FILES=($PACKAGE/Callables.java \
              $PACKAGE/Check.java \
              $PACKAGE/Iterator.java \
              $PACKAGE/Map.java \
              $PACKAGE/Options.java)

MAJOR_VERSION=$(grep MAJOR_VERSION src/cpp/multimap/version.hpp | cut -d';' -f1 | cut -d' ' -f6)
MINOR_VERSION=$(grep MINOR_VERSION src/cpp/multimap/version.hpp | cut -d';' -f1 | cut -d' ' -f6)
PATCH_VERSION=$(grep PATCH_VERSION src/cpp/multimap/version.hpp | cut -d';' -f1 | cut -d' ' -f6)
JAR_FILE=multimap-$MAJOR_VERSION.$MINOR_VERSION.$PATCH_VERSION.jar

mkdir $JAR_DIR
cp -rf $SOURCE_DIR/* $JAR_DIR
cd $JAR_DIR
javac ${SOURCE_FILES[*]}
jar cf ../$JAR_FILE *
cd ..
rm -rf $JAR_DIR
