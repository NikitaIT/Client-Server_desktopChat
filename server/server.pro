#-------------------------------------------------
#
# Project created by QtCreator 2011-10-13T18:08:44
#
#-------------------------------------------------

QT       += core gui network

TARGET = server
TEMPLATE = app


SOURCES += main.cpp\
        dialog.cpp \
    myserver.cpp \
    myclient.cpp

HEADERS  += dialog.h \
    myserver.h \
    myclient.h

FORMS    += dialog.ui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets












