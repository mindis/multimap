TEMPLATE = app
TARGET = multimap
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++11  # for Qt4 compatibility

SOURCES += src/cpp/multimap/command_line_tool.cpp

unix: LIBS += -lboost_system -lmultimap

unix {
    target.path = /usr/local/bin
    INSTALLS += target
}

macx {
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib
}
