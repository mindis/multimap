# For qmake 4.x

CONFIG -= qt
DEFINES -= QT_WEBKIT

QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -march=native
# https://wiki.gentoo.org/wiki/GCC_optimization

QMAKE_LFLAGS += -rdynamic
# Needed for GNU backtrace

INCLUDEPATH += \
    $$PWD/src/cpp

HEADERS += \
    $$PWD/src/cpp/multimap/internal/Arena.hpp \
    $$PWD/src/cpp/multimap/internal/Base64.hpp \
    $$PWD/src/cpp/multimap/internal/Block.hpp \
    $$PWD/src/cpp/multimap/internal/Callbacks.hpp \
    $$PWD/src/cpp/multimap/internal/Iterator.hpp \
    $$PWD/src/cpp/multimap/internal/List.hpp \
    $$PWD/src/cpp/multimap/internal/ListLock.hpp \
    $$PWD/src/cpp/multimap/internal/Shard.hpp \
    $$PWD/src/cpp/multimap/internal/Store.hpp \
    $$PWD/src/cpp/multimap/internal/System.hpp \
    $$PWD/src/cpp/multimap/internal/Table.hpp \
    $$PWD/src/cpp/multimap/internal/UintVector.hpp \
    $$PWD/src/cpp/multimap/internal/Varint.hpp \
    $$PWD/src/cpp/multimap/thirdparty/mt.hpp \
    $$PWD/src/cpp/multimap/thirdparty/xxhash.h \
    $$PWD/src/cpp/multimap/Bytes.hpp \
    $$PWD/src/cpp/multimap/Callables.hpp \
    $$PWD/src/cpp/multimap/Map.hpp \
    $$PWD/src/cpp/multimap/Options.hpp

SOURCES += \
    $$PWD/src/cpp/multimap/internal/Arena.cpp \
    $$PWD/src/cpp/multimap/internal/Base64.cpp \
    $$PWD/src/cpp/multimap/internal/Block.cpp \
    $$PWD/src/cpp/multimap/internal/List.cpp \
    $$PWD/src/cpp/multimap/internal/Shard.cpp \
    $$PWD/src/cpp/multimap/internal/Store.cpp \
    $$PWD/src/cpp/multimap/internal/System.cpp \
    $$PWD/src/cpp/multimap/internal/Table.cpp \
    $$PWD/src/cpp/multimap/internal/UintVector.cpp \
    $$PWD/src/cpp/multimap/internal/Varint.cpp \
    $$PWD/src/cpp/multimap/thirdparty/mt.cpp \
    $$PWD/src/cpp/multimap/thirdparty/xxhash.c \
    $$PWD/src/cpp/multimap/Map.cpp

unix|win32: LIBS += -lboost_filesystem -lboost_system -lboost_thread -pthread
