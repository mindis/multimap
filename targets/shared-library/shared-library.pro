TEMPLATE = lib
TARGET = multimap
VERSION = 0.1

COMMON = multimap.pri
!include(../../$$COMMON) {
    error("Could not find $$COMMON file")
}
