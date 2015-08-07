TEMPLATE = lib
TARGET = multimap-jni
VERSION = 0.1

COMMON = multimap.pri
!include(../../$$COMMON) {
  error("Could not find $$COMMON file")
}

JAVA_HOME = $$(JAVA_HOME)
isEmpty(JAVA_HOME) {
  error("JAVA_HOME is undefined")
}

INCLUDEPATH += \
  $$JAVA_HOME/include

HEADERS += \
  ../../sources/cpp/multimap/jni/common.hpp \
  ../../sources/cpp/multimap/jni/io_multimap_Map_Native.h \
  ../../sources/cpp/multimap/jni/io_multimap_Map_MutableListIter_Native.h \
  ../../sources/cpp/multimap/jni/io_multimap_Map_ImmutableListIter_Native.h

SOURCES += \
  ../../sources/cpp/multimap/jni/io_multimap_Map_Native.cpp \
  ../../sources/cpp/multimap/jni/io_multimap_Map_ImmutableListIter_Native.cpp \
  ../../sources/cpp/multimap/jni/io_multimap_Map_MutableListIter.cpp

unix {
  target.path = /usr/lib
  INSTALLS += target
}
