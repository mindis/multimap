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

    multimap_thirdparty_cmph.path = $$INCLUDE_MULTIMAP/thirdparty/cmph
    multimap_thirdparty_cmph.files += src/cpp/multimap/thirdparty/cmph/cmph.h
    multimap_thirdparty_cmph.files += src/cpp/multimap/thirdparty/cmph/cmph_time.h
    multimap_thirdparty_cmph.files += src/cpp/multimap/thirdparty/cmph/cmph_types.h
    INSTALLS += multimap_thirdparty_cmph

    multimap_thirdparty_mt.path = $$INCLUDE_MULTIMAP/thirdparty/mt
    multimap_thirdparty_mt.files += src/cpp/multimap/thirdparty/mt/mt.hpp
    INSTALLS += multimap_thirdparty_mt

    multimap_thirdparty_xxhash.path = $$INCLUDE_MULTIMAP/thirdparty/xxhash
    multimap_thirdparty_xxhash.files += src/cpp/multimap/thirdparty/xxhash/xxhash.h
    INSTALLS += multimap_thirdparty_xxhash

    target.path = /usr/local/lib
    INSTALLS += target
}
