TEMPLATE = app
TARGET = multimap
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++11

SOURCES += ../src/cpp/multimap/main.cpp

unix: LIBS += -lboost_filesystem -lboost_system -lmultimap

unix {
    target.path = /usr/local/bin
    INSTALLS += target
}
