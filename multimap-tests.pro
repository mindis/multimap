TEMPLATE = app

COMMON = multimap.pri
!include($$COMMON) {
    error("Could not find $$COMMON file")
}

CONFIG += warn_off
QMAKE_CXXFLAGS += -Wall -Wno-sign-compare
# This first removes "-Wall" from the compiler options and then puts it back,
# but with an exception for "-Wno-sign-compare".  The order of flags matters.
# Compiling Google Test emits sign compare warnings we want to suppress.

#DEFINES += MULTIMAP_RUN_LARGE_TESTS

INCLUDEPATH += \
    src/cpp/multimap/thirdparty/googlemock \
    src/cpp/multimap/thirdparty/googlemock/include \
    src/cpp/multimap/thirdparty/googletest \
    src/cpp/multimap/thirdparty/googletest/include

HEADERS += \
    src/cpp/multimap/internal/Generator.hpp \
    src/cpp/multimap/thirdparty/googlemock/include/gmock/internal/custom/gmock-generated-actions.h \
    src/cpp/multimap/thirdparty/googlemock/include/gmock/internal/custom/gmock-matchers.h \
    src/cpp/multimap/thirdparty/googlemock/include/gmock/internal/custom/gmock-port.h \
    src/cpp/multimap/thirdparty/googlemock/include/gmock/internal/gmock-generated-internal-utils.h \
    src/cpp/multimap/thirdparty/googlemock/include/gmock/internal/gmock-internal-utils.h \
    src/cpp/multimap/thirdparty/googlemock/include/gmock/internal/gmock-port.h \
    src/cpp/multimap/thirdparty/googlemock/include/gmock/gmock-actions.h \
    src/cpp/multimap/thirdparty/googlemock/include/gmock/gmock-cardinalities.h \
    src/cpp/multimap/thirdparty/googlemock/include/gmock/gmock-generated-actions.h \
    src/cpp/multimap/thirdparty/googlemock/include/gmock/gmock-generated-function-mockers.h \
    src/cpp/multimap/thirdparty/googlemock/include/gmock/gmock-generated-matchers.h \
    src/cpp/multimap/thirdparty/googlemock/include/gmock/gmock-generated-nice-strict.h \
    src/cpp/multimap/thirdparty/googlemock/include/gmock/gmock-matchers.h \
    src/cpp/multimap/thirdparty/googlemock/include/gmock/gmock-more-actions.h \
    src/cpp/multimap/thirdparty/googlemock/include/gmock/gmock-more-matchers.h \
    src/cpp/multimap/thirdparty/googlemock/include/gmock/gmock-spec-builders.h \
    src/cpp/multimap/thirdparty/googlemock/include/gmock/gmock.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/internal/custom/gtest-port.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/internal/custom/gtest-printers.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/internal/custom/gtest.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/internal/gtest-death-test-internal.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/internal/gtest-filepath.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/internal/gtest-internal.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/internal/gtest-linked_ptr.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/internal/gtest-param-util-generated.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/internal/gtest-param-util.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/internal/gtest-port-arch.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/internal/gtest-port.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/internal/gtest-string.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/internal/gtest-tuple.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/internal/gtest-type-util.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/gtest_pred_impl.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/gtest_prod.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/gtest-death-test.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/gtest-message.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/gtest-param-test.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/gtest-printers.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/gtest-spi.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/gtest-test-part.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/gtest-typed-test.h \
    src/cpp/multimap/thirdparty/googletest/include/gtest/gtest.h \
    src/cpp/multimap/thirdparty/googletest/src/gtest-internal-inl.h

SOURCES += \
    src/cpp/multimap/internal/Base64Test.cpp \
    src/cpp/multimap/internal/ListTest.cpp \
    src/cpp/multimap/internal/DescriptorTest.cpp \
    src/cpp/multimap/internal/MphTableTest.cpp \
    src/cpp/multimap/internal/MphTest.cpp \
    src/cpp/multimap/internal/PartitionTest.cpp \
    src/cpp/multimap/internal/StoreTest.cpp \
    src/cpp/multimap/internal/VarintTest.cpp \
    src/cpp/multimap/internal/UintVectorTest.cpp \
    src/cpp/multimap/thirdparty/googlemock/src/gmock_main.cc \
    src/cpp/multimap/thirdparty/googlemock/src/gmock-cardinalities.cc \
    src/cpp/multimap/thirdparty/googlemock/src/gmock-internal-utils.cc \
    src/cpp/multimap/thirdparty/googlemock/src/gmock-matchers.cc \
    src/cpp/multimap/thirdparty/googlemock/src/gmock-spec-builders.cc \
    src/cpp/multimap/thirdparty/googlemock/src/gmock.cc \
    src/cpp/multimap/thirdparty/googletest/src/gtest-death-test.cc \
    src/cpp/multimap/thirdparty/googletest/src/gtest-filepath.cc \
    src/cpp/multimap/thirdparty/googletest/src/gtest-port.cc \
    src/cpp/multimap/thirdparty/googletest/src/gtest-printers.cc \
    src/cpp/multimap/thirdparty/googletest/src/gtest-test-part.cc \
    src/cpp/multimap/thirdparty/googletest/src/gtest-typed-test.cc \
    src/cpp/multimap/thirdparty/googletest/src/gtest.cc \
    src/cpp/multimap/ArenaTest.cpp \
    src/cpp/multimap/MapTest.cpp \
    src/cpp/multimap/SliceTest.cpp \
    src/cpp/multimap/VersionTest.cpp

CONFIG(debug, debug|release) {
    TARGET = multimap-tests-dbg
} else {
    TARGET = multimap-tests
}

# Only enable for memory leak checking with Google Address Sanitizer.
# Caution: You may experience increased memory usage.
# QMAKE_CXXFLAGS_DEBUG += -fsanitize=address -fno-omit-frame-pointer
# QMAKE_LFLAGS_DEBUG += -fsanitize=address

# Eventually export the following before running the executable.
# export ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer-3.5
# export ASAN_OPTIONS=symbolize=1
# ./<excutable> [params]

# Only enable for profiling with Google CPU Profiler.
# LIBS += -lprofiler

# Only enable for profiling with Google Heap Profiler.
# LIBS += -ltcmalloc
