CONFIG -= qt
CONFIG += c++11
DEFINES -= QT_WEBKIT
VERSION = 0.5.0

QMAKE_CXXFLAGS += -std=c++11  # for Qt4 compatibility
QMAKE_LFLAGS += -rdynamic     # for GNU backtrace

INCLUDEPATH += \
    src/cpp \
    src/cpp/multimap/thirdparty

HEADERS += \
    src/cpp/multimap/internal/Arena.hpp \
    src/cpp/multimap/internal/Base64.hpp \
    src/cpp/multimap/internal/Block.hpp \
    src/cpp/multimap/internal/List.hpp \
    src/cpp/multimap/internal/Locks.hpp \
    src/cpp/multimap/internal/Partition.hpp \
    src/cpp/multimap/internal/SharedMutex.hpp \
    src/cpp/multimap/internal/Stats.hpp \
    src/cpp/multimap/internal/Store.hpp \
    src/cpp/multimap/internal/UintVector.hpp \
    src/cpp/multimap/internal/Varint.hpp \
    src/cpp/multimap/thirdparty/mt/mt.hpp \
    src/cpp/multimap/thirdparty/xxhash/xxhash.h \
    src/cpp/multimap/Bytes.hpp \
    src/cpp/multimap/ImmutableMap.hpp \
    src/cpp/multimap/Iterator.hpp \
    src/cpp/multimap/Map.hpp \
    src/cpp/multimap/Version.hpp \
    src/cpp/multimap/internal/Mph.hpp \
    src/cpp/multimap/internal/MphTable.hpp

SOURCES += \
    src/cpp/multimap/internal/Arena.cpp \
    src/cpp/multimap/internal/Base64.cpp \
    src/cpp/multimap/internal/List.cpp \
    src/cpp/multimap/internal/Partition.cpp \
    src/cpp/multimap/internal/SharedMutex.cpp \
    src/cpp/multimap/internal/Stats.cpp \
    src/cpp/multimap/internal/Store.cpp \
    src/cpp/multimap/internal/UintVector.cpp \
    src/cpp/multimap/internal/Varint.cpp \
    src/cpp/multimap/thirdparty/mt/mt.cpp \
    src/cpp/multimap/thirdparty/xxhash/xxhash.c \
    src/cpp/multimap/Iterator.cpp \
    src/cpp/multimap/ImmutableMap.cpp \
    src/cpp/multimap/Map.cpp \
    src/cpp/multimap/Version.cpp \
    src/cpp/multimap/internal/Mph.cpp \
    src/cpp/multimap/internal/MphTable.cpp

unix:!macx: LIBS += -lboost_filesystem -lboost_system -lboost_thread -lpthread

macx {
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib -lboost_filesystem -lboost_system -lboost_thread-mt
}
