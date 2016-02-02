TARGET = multimap
TEMPLATE = lib

COMMON = multimap.pri
!include($$COMMON) {
    error("Could not find $$COMMON file")
}

unix {
    INCLUDE_MULTIMAP = /usr/local/include/multimap

    multimap.path = $$INCLUDE_MULTIMAP
    multimap.files += src/cpp/multimap/*.h*
    INSTALLS += multimap

    multimap_internal.path = $$INCLUDE_MULTIMAP/internal
    multimap_internal.files += src/cpp/multimap/internal/*.h*
    INSTALLS += multimap_internal

    multimap_thirdparty_mt.path = $$INCLUDE_MULTIMAP/thirdparty/mt
    multimap_thirdparty_mt.files += src/cpp/multimap/thirdparty/mt/*.h*
    INSTALLS += multimap_thirdparty_mt

    multimap_thirdparty_xxhash.path = $$INCLUDE_MULTIMAP/thirdparty/xxhash
    multimap_thirdparty_xxhash.files += src/cpp/multimap/thirdparty/xxhash/*.h*
    INSTALLS += multimap_thirdparty_xxhash

    target.path = /usr/local/lib
    INSTALLS += target
}
