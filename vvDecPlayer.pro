QT += core gui widgets openglwidgets opengl xml concurrent network

TARGET = vvDecPlayer
TEMPLATE = app
CONFIG += c++1z
CONFIG -= debug_and_release

SOURCES += $$files(src/*.cpp, true)
HEADERS += $$files(src/*.h, true)
FORMS += $$files(ui/*.ui, false)

INCLUDEPATH += src/
