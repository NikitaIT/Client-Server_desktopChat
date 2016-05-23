#-------------------------------------------------
#
# Project created by QtCreator 2011-10-13T22:35:09
#
#-------------------------------------------------

QT       += core gui network

TARGET = client
TEMPLATE = app


SOURCES += main.cpp\
        dialog_client.cpp

HEADERS  += dialog_client.h \


FORMS    += dialog_client.ui



greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
