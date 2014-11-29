TARGET   = basic_aps_plugin

# common configuration for deCONZ plugins

TARGET = $$qtLibraryTarget($$TARGET)

DEFINES += DECONZ_DLLSPEC=Q_DECL_IMPORT

win32:LIBS+=  -L../.. -ldeCONZ1
unix:LIBS+=  -L../.. -ldeCONZ -lcrypt
win32:CONFIG += dll

TEMPLATE        = lib
CONFIG         += plugin \
               += debug_and_release

INCLUDEPATH    += ../.. \
                  ../../common

unix:INCLUDEPATH += /usr/include

QMAKE_CXXFLAGS += -Wno-attributes

HEADERS  = basic_aps_plugin.h

SOURCES  = basic_aps_plugin.cpp
