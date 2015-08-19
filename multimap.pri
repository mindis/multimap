# For qmake 4.x

CONFIG -= qt
DEFINES -= QT_WEBKIT

QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -march=native
# https://wiki.gentoo.org/wiki/GCC_optimization

INCLUDEPATH += \
    $$PWD/sources/cpp

HEADERS += \
    $$PWD/sources/cpp/multimap/internal/AutoCloseFile.hpp \
    $$PWD/sources/cpp/multimap/internal/Arena.hpp \
    $$PWD/sources/cpp/multimap/internal/Block.hpp \
    $$PWD/sources/cpp/multimap/internal/BlockPool.hpp \
    $$PWD/sources/cpp/multimap/internal/Callbacks.hpp \
    $$PWD/sources/cpp/multimap/internal/Check.hpp \
    $$PWD/sources/cpp/multimap/internal/DataFile.hpp \
    $$PWD/sources/cpp/multimap/internal/List.hpp \
    $$PWD/sources/cpp/multimap/internal/ListLock.hpp \
    $$PWD/sources/cpp/multimap/internal/System.hpp \
    $$PWD/sources/cpp/multimap/internal/Table.hpp \
    $$PWD/sources/cpp/multimap/internal/UintVector.hpp \
    $$PWD/sources/cpp/multimap/internal/Varint.hpp \
    $$PWD/sources/cpp/multimap/internal/thirdparty/farmhash.h \
    $$PWD/sources/cpp/multimap/Bytes.hpp \
    $$PWD/sources/cpp/multimap/Iterator.hpp \
    $$PWD/sources/cpp/multimap/Options.hpp \
    $$PWD/sources/cpp/multimap/Map.hpp \
    $$PWD/sources/cpp/multimap/Callables.hpp \

SOURCES += \
    $$PWD/sources/cpp/multimap/internal/Arena.cpp \
    $$PWD/sources/cpp/multimap/internal/Check.cpp \
    $$PWD/sources/cpp/multimap/internal/Block.cpp \
    $$PWD/sources/cpp/multimap/internal/BlockPool.cpp \
    $$PWD/sources/cpp/multimap/internal/DataFile.cpp \
    $$PWD/sources/cpp/multimap/internal/List.cpp \
    $$PWD/sources/cpp/multimap/internal/System.cpp \
    $$PWD/sources/cpp/multimap/internal/Table.cpp \
    $$PWD/sources/cpp/multimap/internal/UintVector.cpp \
    $$PWD/sources/cpp/multimap/internal/Varint.cpp \
    $$PWD/sources/cpp/multimap/internal/thirdparty/farmhash.cc \
    $$PWD/sources/cpp/multimap/Map.cpp

unix|win32: LIBS += -lboost_filesystem -lboost_system -lboost_thread -pthread
