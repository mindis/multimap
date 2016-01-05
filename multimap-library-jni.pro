TEMPLATE = lib
TARGET = multimap-jni
VERSION = 0.3

COMMON = multimap.pri
!include($$COMMON) {
    error("Could not find $$COMMON file")
}

JAVA_HOME = $$(JAVA_HOME)
isEmpty(JAVA_HOME) {
    error("JAVA_HOME is undefined, but required if you want to build multimap-library-jni.pro")
}

INCLUDEPATH += \
    $$JAVA_HOME/include

HEADERS += \
    src/cpp/multimap/jni/generated/io_multimap_Iterator_Native.h \
    src/cpp/multimap/jni/generated/io_multimap_Map_Limits_Native.h \
    src/cpp/multimap/jni/generated/io_multimap_Map_Native.h \
    src/cpp/multimap/jni/common.hpp

SOURCES += \
    src/cpp/multimap/jni/common.cpp \
    src/cpp/multimap/jni/io_multimap_Iterator_Native.cpp \
    src/cpp/multimap/jni/io_multimap_Map_Limits_Native.cpp \
    src/cpp/multimap/jni/io_multimap_Map_Native.cpp

unix {
    target.path = /usr/local/lib
    INSTALLS += target
}
