MULTIMAP_LIBRARY = multimap-library.pro
!include($$MULTIMAP_LIBRARY) {
    error("Could not find $$MULTIMAP_LIBRARY file")
}

CONFIG += debug
TARGET = multimap-dbg
