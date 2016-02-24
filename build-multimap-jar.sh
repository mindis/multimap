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
              $PACKAGE/Utils.java)

MAJOR_VERSION=$(grep MAJOR src/cpp/multimap/Version.hpp | grep -Po '[0-9]+(?=;)')
MINOR_VERSION=$(grep MINOR src/cpp/multimap/Version.hpp | grep -Po '[0-9]+(?=;)')
PATCH_VERSION=$(grep PATCH src/cpp/multimap/Version.hpp | grep -Po '[0-9]+(?=;)')
JAR_FILE=multimap-$MAJOR_VERSION.$MINOR_VERSION.$PATCH_VERSION.jar

mkdir $JAR_DIR
cp -rf $SOURCE_DIR/* $JAR_DIR
cd $JAR_DIR
javac ${SOURCE_FILES[*]}
jar cf ../$JAR_FILE *
cd ..
rm -rf $JAR_DIR
