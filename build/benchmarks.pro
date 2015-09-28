TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

COMMON = multimap.pri
!include(../$$COMMON) {
    error("Could not find $$COMMON file")
}

HEADERS += \
    ../src/cpp/multimap/internal/Generator.hpp

SOURCES += \
    ../src/cpp/multimap/RunBenchmarks.cpp

unix: LIBS += -lboost_program_options -lprofiler
