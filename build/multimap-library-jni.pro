TEMPLATE = lib
TARGET = multimap-jni
VERSION = 0.3

COMMON = multimap.pri
!include(../$$COMMON) {
    error("Could not find $$COMMON file")
}

JAVA_HOME = $$(JAVA_HOME)
isEmpty(JAVA_HOME) {
    error("JAVA_HOME is undefined")
}

INCLUDEPATH += \
    $$JAVA_HOME/include

HEADERS += \
    ../src/cpp/multimap/jni/common.hpp \
    ../src/cpp/multimap/jni/io_multimap_Map_Native.h \
    ../src/cpp/multimap/jni/io_multimap_Map_MutableListIter_Native.h \
    ../src/cpp/multimap/jni/io_multimap_Map_ImmutableListIter_Native.h

SOURCES += \
    ../src/cpp/multimap/jni/io_multimap_Map_Native.cpp \
    ../src/cpp/multimap/jni/io_multimap_Map_ImmutableListIter_Native.cpp \
    ../src/cpp/multimap/jni/io_multimap_Map_MutableListIter.cpp

unix {
    target.path = /usr/lib
    INSTALLS += target
}
