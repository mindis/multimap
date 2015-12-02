CONFIG -= qt
DEFINES -= QT_WEBKIT

QMAKE_CXXFLAGS += -std=c++11
QMAKE_LFLAGS += -rdynamic
# Needed for GNU backtrace

INCLUDEPATH += \
    $$PWD/src/cpp \
    $$PWD/src/cpp/multimap/thirdparty

HEADERS += \
    $$PWD/src/cpp/multimap/internal/Arena.hpp \
    $$PWD/src/cpp/multimap/internal/Base64.hpp \
    $$PWD/src/cpp/multimap/internal/Block.hpp \
    $$PWD/src/cpp/multimap/internal/List.hpp \
    $$PWD/src/cpp/multimap/internal/Shard.hpp \
    $$PWD/src/cpp/multimap/internal/Store.hpp \
    $$PWD/src/cpp/multimap/internal/UintVector.hpp \
    $$PWD/src/cpp/multimap/internal/Varint.hpp \
    $$PWD/src/cpp/multimap/thirdparty/mt/mt.hpp \
    $$PWD/src/cpp/multimap/thirdparty/xxhash/xxhash.h \
    $$PWD/src/cpp/multimap/Bytes.hpp \
    $$PWD/src/cpp/multimap/Callables.hpp \
    $$PWD/src/cpp/multimap/Map.hpp \
    $$PWD/src/cpp/multimap/Options.hpp

SOURCES += \
    $$PWD/src/cpp/multimap/internal/Arena.cpp \
    $$PWD/src/cpp/multimap/internal/Base64.cpp \
    $$PWD/src/cpp/multimap/internal/List.cpp \
    $$PWD/src/cpp/multimap/internal/Shard.cpp \
    $$PWD/src/cpp/multimap/internal/Store.cpp \
    $$PWD/src/cpp/multimap/internal/UintVector.cpp \
    $$PWD/src/cpp/multimap/internal/Varint.cpp \
    $$PWD/src/cpp/multimap/thirdparty/mt/mt.cpp \
    $$PWD/src/cpp/multimap/thirdparty/xxhash/xxhash.c \
    $$PWD/src/cpp/multimap/Map.cpp

unix|win32: LIBS += -lboost_filesystem -lboost_system -lboost_thread -pthread
