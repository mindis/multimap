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
    ../src/cpp/multimap/main-tool.cpp
