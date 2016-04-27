TEMPLATE = lib

COMMON = multimap.pri
!include($$COMMON) {
    error("Could not find $$COMMON file")
}

JAVA_HOME = $$(JAVA_HOME)
isEmpty(JAVA_HOME) {
    error("JAVA_HOME is undefined, but required in order to build this target."\
          "Please run e.g. 'export JAVA_HOME=/usr/lib/jvm/default-java' an try again.")
}

INCLUDEPATH += \
    $$JAVA_HOME/include \
    $$JAVA_HOME/include/linux \
    $$JAVA_HOME/include/darwin

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

CONFIG(debug, debug|release) {
    TARGET = multimap-jni-dbg
} else {
    TARGET = multimap-jni
}

unix {
    target.path = /usr/local/lib
    INSTALLS += target
}
