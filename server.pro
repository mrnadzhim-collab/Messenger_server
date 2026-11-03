QT += core network sql
QT -= gui

CONFIG += console c++11

DESTDIR= /usr/local/bin

TARGET = server

TEMPLATE = app

SOURCES += \
        main.cpp \
        server.cpp


HEADERS += \
    errors.h \
    protocol.h \
    server.h \
    types.h
