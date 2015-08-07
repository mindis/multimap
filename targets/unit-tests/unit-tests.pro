TEMPLATE = app
CONFIG += console

COMMON = multimap.pri
!include(../../$$COMMON) {
    error("Could not find $$COMMON file")
}

INCLUDEPATH += /usr/src/gtest

HEADERS += \
    $$PWD/../../sources/cpp/multimap/internal/Generator.hpp

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
    ../../sources/cpp/multimap/internal/BlockPoolTest.cpp \
    ../../sources/cpp/multimap/internal/BlockTest.cpp \
    ../../sources/cpp/multimap/internal/ListLockTest.cpp \
    ../../sources/cpp/multimap/internal/ListTest.cpp \
    ../../sources/cpp/multimap/internal/TableTest.cpp \
    ../../sources/cpp/multimap/internal/UintVectorTest.cpp \
    ../../sources/cpp/multimap/internal/VarintTest.cpp \
    ../../sources/cpp/multimap/IteratorTest.cpp \
    ../../sources/cpp/multimap/MapTest.cpp \
    ../../sources/cpp/multimap/RunAllTests.cpp

#unix {
#    OBJECTS_DIR = $$PWD
#    DESTDIR = $$PWD
#    OUT_PWD = $$PWD
#}
