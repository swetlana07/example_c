#-------------------------------------------------
#
# Project created by QtCreator 2021-04-12T09:31:47
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = sev_mt
CONFIG   += console
CONFIG   -= app_bundle
RELEASE:{
QMAKE_CXXFLAGS_RELEASE += +O0
}
TEMPLATE = app


SOURCES += WDMTMKv2.cpp\
           main.cpp


HEADERS  += WDMTMKv2.h
