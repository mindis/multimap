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
    src/cpp/multimap/internal/MapPartition.hpp \
    src/cpp/multimap/internal/SharedMutex.hpp \
    src/cpp/multimap/internal/Stats.hpp \
    src/cpp/multimap/internal/Store.hpp \
    src/cpp/multimap/internal/UintVector.hpp \
    src/cpp/multimap/internal/Varint.hpp \
    src/cpp/multimap/thirdparty/mt/mt.hpp \
    src/cpp/multimap/thirdparty/xxhash/xxhash.h \
    src/cpp/multimap/Bytes.hpp \
    src/cpp/multimap/callables.hpp \
    src/cpp/multimap/Iterator.hpp \
    src/cpp/multimap/Map.hpp \
    src/cpp/multimap/Options.hpp \
    src/cpp/multimap/Version.hpp

SOURCES += \
    src/cpp/multimap/internal/Arena.cpp \
    src/cpp/multimap/internal/Base64.cpp \
    src/cpp/multimap/internal/List.cpp \
    src/cpp/multimap/internal/MapPartition.cpp \
    src/cpp/multimap/internal/SharedMutex.cpp \
    src/cpp/multimap/internal/Stats.cpp \
    src/cpp/multimap/internal/Store.cpp \
    src/cpp/multimap/internal/UintVector.cpp \
    src/cpp/multimap/internal/Varint.cpp \
    src/cpp/multimap/thirdparty/mt/mt.cpp \
    src/cpp/multimap/thirdparty/xxhash/xxhash.c \
    src/cpp/multimap/Map.cpp

unix:!macx: LIBS += -lboost_filesystem -lboost_system -lboost_thread -pthread

macx {
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib -lboost_filesystem -lboost_system -lboost_thread-mt
}
