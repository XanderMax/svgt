TEMPLATE = lib

CONFIG += qt svg plugin 
QT += qml quick core-private

SOURCES += src/item.cpp src/engine.cpp
HEADERS += src/item.h src/engine.h src/plugin.hpp

TARGET = qmlsvgtemplate

DISTFILES += qmldir

DESTDIR = svgt/

