TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++11  # for Qt4 compatibility

INCLUDEPATH += $$PWD/src/cpp
DEPENDPATH += $$PWD/src/cpp

SOURCES += src/cpp/multimap/command_line_tool.cpp

macx {
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib
}

unix: LIBS += -lmultimap -lboost_system

unix {
    target.path = /usr/local/bin
    INSTALLS += target
}

CONFIG(debug, debug|release) {
    TARGET = multimap-dbg
} else {
    TARGET = multimap
}
