QT += core gui widgets opengl xml concurrent network

TARGET = vvDecPlayer
TEMPLATE = app
CONFIG += c++11
CONFIG -= debug_and_release

SOURCES += $$files(src/*.cpp, false)
HEADERS += $$files(src/*.h, false)
FORMS += $$files(ui/*.ui, false)

INCLUDEPATH += src/
