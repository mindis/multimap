TEMPLATE = app
TARGET = multimap
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

COMMON = multimap.pri
!include(../$$COMMON) {
    error("Could not find $$COMMON file")
}

SOURCES += \
    ../src/cpp/multimap/RunCommandLineTool.cpp

unix: LIBS += -lboost_program_options
