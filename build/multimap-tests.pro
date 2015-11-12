TEMPLATE = app
TARGET = multimap-tests
CONFIG += console

COMMON = multimap.pri
!include(../$$COMMON) {
    error("Could not find $$COMMON file")
}

INCLUDEPATH += /usr/src/gtest

HEADERS += \
    ../src/cpp/multimap/internal/Generator.hpp

# sudo apt-get install libgtest-dev
# sudo apt-get install google-mock
SOURCES += \
    /usr/src/gmock/src/gmock-cardinalities.cc \
    /usr/src/gmock/src/gmock-internal-utils.cc \
    /usr/src/gmock/src/gmock-matchers.cc \
    /usr/src/gmock/src/gmock-spec-builders.cc \
    /usr/src/gmock/src/gmock.cc \
    /usr/src/gtest/src/gtest-death-test.cc \
    /usr/src/gtest/src/gtest-filepath.cc \
    /usr/src/gtest/src/gtest-port.cc \
    /usr/src/gtest/src/gtest-printers.cc \
    /usr/src/gtest/src/gtest-test-part.cc \
    /usr/src/gtest/src/gtest-typed-test.cc \
    /usr/src/gtest/src/gtest.cc \
    ../src/cpp/multimap/internal/ArenaTest.cpp \
    ../src/cpp/multimap/internal/Base64Test.cpp \
    ../src/cpp/multimap/internal/BlockTest.cpp \
    ../src/cpp/multimap/internal/ListTest.cpp \
    ../src/cpp/multimap/internal/TableTest.cpp \
    ../src/cpp/multimap/internal/SystemTest.cpp \
    ../src/cpp/multimap/internal/UintVectorTest.cpp \
    ../src/cpp/multimap/internal/VarintTest.cpp \
    ../src/cpp/multimap/MapTest.cpp \
    ../src/cpp/multimap/RunUnitTests.cpp
