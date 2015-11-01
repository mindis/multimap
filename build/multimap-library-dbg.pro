TEMPLATE = lib
TARGET = multimap-dbg
VERSION = 0.2
CONFIG += debug

COMMON = multimap.pri
!include(../$$COMMON) {
    error("Could not find $$COMMON file")
}

unix {
    multimap.path = /usr/include/multimap
    multimap.files += ../src/cpp/multimap/*.h
    multimap.files += ../src/cpp/multimap/*.hpp
    INSTALLS += multimap

    multimap_internal.path = /usr/include/multimap/internal
    multimap_internal.files += ../src/cpp/multimap/internal/*.h
    multimap_internal.files += ../src/cpp/multimap/internal/*.hpp
    INSTALLS += multimap_internal

    multimap_thirdparty.path = /usr/include/multimap/thirdparty
    multimap_thirdparty.files += ../src/cpp/multimap/thirdparty/*.h
    multimap_thirdparty.files += ../src/cpp/multimap/thirdparty/*.hpp
    INSTALLS += multimap_thirdparty

    target.path = /usr/lib
    INSTALLS += target
}
