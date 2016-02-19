MULTIMAP_LIBRARY_JNI = multimap-library-jni.pro
!include($$MULTIMAP_LIBRARY_JNI) {
    error("Could not find $$MULTIMAP_LIBRARY_JNI file")
}

CONFIG += debug
TARGET = multimap-jni-dbg
