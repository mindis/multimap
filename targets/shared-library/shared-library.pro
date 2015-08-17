TEMPLATE = lib
TARGET = multimap
VERSION = 0.2

COMMON = multimap.pri
!include(../../$$COMMON) {
  error("Could not find $$COMMON file")
}

unix {
  multimap.path = /usr/include/multimap
  multimap.files += ../../sources/cpp/multimap/*.h
  multimap.files += ../../sources/cpp/multimap/*.hpp
  INSTALLS += multimap

  multimap_internal.path = /usr/include/multimap/internal
  multimap_internal.files += ../../sources/cpp/multimap/internal/*.h
  multimap_internal.files += ../../sources/cpp/multimap/internal/*.hpp
  INSTALLS += multimap_internal

  multimap_internal_thirdparty.path = /usr/include/multimap/internal/thirdparty
  multimap_internal_thirdparty.files += ../../sources/cpp/multimap/internal/thirdparty/*.h
  multimap_internal_thirdparty.files += ../../sources/cpp/multimap/internal/thirdparty/*.hpp
  INSTALLS += multimap_internal_thirdparty

  target.path = /usr/lib
  INSTALLS += target
}
