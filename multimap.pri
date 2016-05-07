CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
VERSION = 0.6.0

QMAKE_LFLAGS += -rdynamic # for GNU backtrace

INCLUDEPATH += \
    src/cpp \
    src/cpp/multimap/thirdparty

HEADERS += \
    src/cpp/multimap/internal/Base64.h \
    src/cpp/multimap/internal/Descriptor.h \
    src/cpp/multimap/internal/List.h \
    src/cpp/multimap/internal/Locks.h \
    src/cpp/multimap/internal/Mph.h \
    src/cpp/multimap/internal/MphTable.h \
    src/cpp/multimap/internal/Partition.h \
    src/cpp/multimap/internal/SharedMutex.h \
    src/cpp/multimap/internal/Store.h \
    src/cpp/multimap/internal/TsvFileReader.h \
    src/cpp/multimap/internal/TsvFileWriter.h \
    src/cpp/multimap/internal/UintVector.h \
    src/cpp/multimap/internal/Varint.h \
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
    src/cpp/multimap/thirdparty/mt/assert.hpp \
    src/cpp/multimap/thirdparty/mt/check.hpp \
    src/cpp/multimap/thirdparty/mt/common.hpp \
    src/cpp/multimap/thirdparty/mt/fileio.hpp \
    src/cpp/multimap/thirdparty/mt/memory.hpp \
    src/cpp/multimap/thirdparty/mt/unicode.hpp \
    src/cpp/multimap/thirdparty/mt/varint.hpp \
    src/cpp/multimap/thirdparty/xxhash/xxhash.h \
    src/cpp/multimap/Arena.h \
    src/cpp/multimap/Bytes.h \
    src/cpp/multimap/callables.h \
    src/cpp/multimap/ImmutableMap.h \
    src/cpp/multimap/Iterator.h \
    src/cpp/multimap/Map.h \
    src/cpp/multimap/Options.h \
    src/cpp/multimap/Slice.h \
    src/cpp/multimap/Stats.h \
    src/cpp/multimap/Version.h

SOURCES += \
    src/cpp/multimap/internal/Base64.cpp \
    src/cpp/multimap/internal/Descriptor.cpp \
    src/cpp/multimap/internal/List.cpp \
    src/cpp/multimap/internal/Mph.cpp \
    src/cpp/multimap/internal/MphTable.cpp \
    src/cpp/multimap/internal/Partition.cpp \
    src/cpp/multimap/internal/SharedMutex.cpp \
    src/cpp/multimap/internal/Store.cpp \
    src/cpp/multimap/internal/TsvFileReader.cpp \
    src/cpp/multimap/internal/TsvFileWriter.cpp \
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
    src/cpp/multimap/thirdparty/mt/assert.cpp \
    src/cpp/multimap/thirdparty/mt/check.cpp \
    src/cpp/multimap/thirdparty/mt/common.cpp \
    src/cpp/multimap/thirdparty/mt/fileio.cpp \
    src/cpp/multimap/thirdparty/mt/memory.cpp \
    src/cpp/multimap/thirdparty/mt/unicode.cpp \
    src/cpp/multimap/thirdparty/mt/varint.cpp \
    src/cpp/multimap/thirdparty/xxhash/xxhash.c \
    src/cpp/multimap/Arena.cpp \
    src/cpp/multimap/Bytes.cpp \
    src/cpp/multimap/ImmutableMap.cpp \
    src/cpp/multimap/Iterator.cpp \
    src/cpp/multimap/Map.cpp \
    src/cpp/multimap/Slice.cpp \
    src/cpp/multimap/Stats.cpp \
    src/cpp/multimap/Version.cpp

OTHER_FILES += \
    install-dependencies-debian.sh \
    run-cpplint.sh \
    update-mt-lib.sh

unix:!macx: LIBS += -lboost_filesystem -lboost_system -lboost_thread -lpthread

macx {
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib -lboost_filesystem -lboost_system -lboost_thread-mt
}
