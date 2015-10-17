TEMPLATE = app
CONFIG += console

COMMON = multimap.pri
!include(../$$COMMON) {
    error("Could not find $$COMMON file")
}

HEADERS += \
    ../src/cpp/multimap/internal/Generator.hpp

SOURCES += \
    ../src/cpp/multimap/RunLevelDbBenchmarks.cpp

unix: LIBS += -lboost_program_options -lleveldb
