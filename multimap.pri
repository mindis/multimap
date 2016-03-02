CONFIG -= qt
CONFIG += c++11
DEFINES -= QT_WEBKIT
VERSION = 0.5.0

QMAKE_LFLAGS += -rdynamic     # for GNU backtrace
QMAKE_CFLAGS += -std=c99      # for Qt4 compatibility
QMAKE_CXXFLAGS += -std=c++11  # for Qt4 compatibility

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
    src/cpp/multimap/thirdparty/cmph/bdz.h \
    src/cpp/multimap/thirdparty/cmph/bdz_ph.h \
    src/cpp/multimap/thirdparty/cmph/bdz_structs.h \
    src/cpp/multimap/thirdparty/cmph/bdz_structs_ph.h \
    src/cpp/multimap/thirdparty/cmph/bitbool.h \
    src/cpp/multimap/thirdparty/cmph/bmz.h \
    src/cpp/multimap/thirdparty/cmph/bmz8.h \
    src/cpp/multimap/thirdparty/cmph/bmz8_structs.h \
    src/cpp/multimap/thirdparty/cmph/bmz_structs.h \
    src/cpp/multimap/thirdparty/cmph/brz.h \
    src/cpp/multimap/thirdparty/cmph/brz_structs.h \
    src/cpp/multimap/thirdparty/cmph/buffer_entry.h \
    src/cpp/multimap/thirdparty/cmph/buffer_manager.h \
    src/cpp/multimap/thirdparty/cmph/chd.h \
    src/cpp/multimap/thirdparty/cmph/chd_ph.h \
    src/cpp/multimap/thirdparty/cmph/chd_structs.h \
    src/cpp/multimap/thirdparty/cmph/chd_structs_ph.h \
    src/cpp/multimap/thirdparty/cmph/chm.h \
    src/cpp/multimap/thirdparty/cmph/chm_structs.h \
    src/cpp/multimap/thirdparty/cmph/cmph.h \
    src/cpp/multimap/thirdparty/cmph/cmph_structs.h \
    src/cpp/multimap/thirdparty/cmph/cmph_time.h \
    src/cpp/multimap/thirdparty/cmph/cmph_types.h \
    src/cpp/multimap/thirdparty/cmph/compressed_rank.h \
    src/cpp/multimap/thirdparty/cmph/compressed_seq.h \
    src/cpp/multimap/thirdparty/cmph/debug.h \
    src/cpp/multimap/thirdparty/cmph/fch.h \
    src/cpp/multimap/thirdparty/cmph/fch_buckets.h \
    src/cpp/multimap/thirdparty/cmph/fch_structs.h \
    src/cpp/multimap/thirdparty/cmph/graph.h \
    src/cpp/multimap/thirdparty/cmph/hash.h \
    src/cpp/multimap/thirdparty/cmph/hash_state.h \
    src/cpp/multimap/thirdparty/cmph/jenkins_hash.h \
    src/cpp/multimap/thirdparty/cmph/miller_rabin.h \
    src/cpp/multimap/thirdparty/cmph/select.h \
    src/cpp/multimap/thirdparty/cmph/select_lookup_tables.h \
    src/cpp/multimap/thirdparty/cmph/vqueue.h \
    src/cpp/multimap/thirdparty/cmph/vstack.h \
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
    src/cpp/multimap/thirdparty/cmph/bdz.c \
    src/cpp/multimap/thirdparty/cmph/bdz_ph.c \
    src/cpp/multimap/thirdparty/cmph/bmz.c \
    src/cpp/multimap/thirdparty/cmph/bmz8.c \
    src/cpp/multimap/thirdparty/cmph/brz.c \
    src/cpp/multimap/thirdparty/cmph/buffer_entry.c \
    src/cpp/multimap/thirdparty/cmph/buffer_manager.c \
    src/cpp/multimap/thirdparty/cmph/chd.c \
    src/cpp/multimap/thirdparty/cmph/chd_ph.c \
    src/cpp/multimap/thirdparty/cmph/chm.c \
    src/cpp/multimap/thirdparty/cmph/cmph.c \
    src/cpp/multimap/thirdparty/cmph/cmph_structs.c \
    src/cpp/multimap/thirdparty/cmph/compressed_rank.c \
    src/cpp/multimap/thirdparty/cmph/compressed_seq.c \
    src/cpp/multimap/thirdparty/cmph/fch.c \
    src/cpp/multimap/thirdparty/cmph/fch_buckets.c \
    src/cpp/multimap/thirdparty/cmph/graph.c \
    src/cpp/multimap/thirdparty/cmph/hash.c \
    src/cpp/multimap/thirdparty/cmph/jenkins_hash.c \
    src/cpp/multimap/thirdparty/cmph/miller_rabin.c \
    src/cpp/multimap/thirdparty/cmph/select.c \
    src/cpp/multimap/thirdparty/cmph/vqueue.c \
    src/cpp/multimap/thirdparty/cmph/vstack.c \
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
