#-------------------------------------------------
#
# Project created by QtCreator 2013-11-01T16:11:04
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = AudioProj
TEMPLATE = app

win32:INCLUDEPATH += $$PWD

SOURCES += main.cpp\
        audiocfg.cpp \
    network.cpp \
    qaudiolevel.cpp

HEADERS  += audiocfg.H \
    MyAudioBase.H \
    network.H \
    qaudiolevel.H

FORMS    += audiocfg.ui

QT += multimedia\
    network


RESOURCES += \
    sometext.qrc
