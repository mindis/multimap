#!/usr/bin/env bash

cpplint --root=src/cpp \
src/cpp/multimap/*.h \
src/cpp/multimap/*.cpp \
src/cpp/multimap/internal/*.h \
src/cpp/multimap/internal/*.cpp
