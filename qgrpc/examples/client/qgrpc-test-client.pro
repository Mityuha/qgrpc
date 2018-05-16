
unix: QMAKE_CXXFLAGS += -std=c++11
unix: QMAKE_LFLAGS_DEBUG += -std=c++11
win32-g++: QMAKE_CXXFLAGS += -std=c++0x 

DEFINES += __STDC_LIMIT_MACROS

QGRPC_CONFIG = client

TEMPLATE = app

TARGET   = qgrpc-test-client

CONFIG += qt
CONFIG += debug_and_release
QT *= widgets

GRPC  +=  ../proto/pingpong.proto

HEADERS +=  qgrpc-test-client.h

SOURCES +=  main.cpp

FORMS += qgrpc-test-client.ui

PWD = ../../..

CONFIG(debug, debug|release) {
    MOC_DIR = debug
} else {
    MOC_DIR = release
}

INC_GRPC = $${PWD}/grpc.pri
!include($${INC_GRPC}) {
		error("$$TARGET: File not found: $${INC_GRPC}")
}

AUTHORS = dmakarov

