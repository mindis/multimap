TEMPLATE = app
TARGET = multimap-benchmarks
CONFIG += console

COMMON = multimap.pri
!include(../../$$COMMON) {
    error("Could not find $$COMMON file")
}

HEADERS += \
    ../../sources/cpp/multimap/internal/Generator.hpp

SOURCES += \
    ../../sources/cpp/multimap/RunBenchmarks.cpp \
    ../../sources/cpp/multimap/internal/Generator.cpp

unix: LIBS += -lboost_program_options
