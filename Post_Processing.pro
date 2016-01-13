#-------------------------------------------------
#
# Project created by QtCreator 2015-12-24T22:14:49
#
#-------------------------------------------------

QT  = opengl
CONFIG += c++11

Debug:TARGET = Post_Processing_debug
Release:TARGET = Post_Processing

TEMPLATE = app

DESTDIR = $$PWD/bin

app_data.path = $${DESTDIR}
app_data.files = $$PWD/data

INSTALLS += app_data

Debug:BUILDDIR      = debug
Release:BUILDDIR    = release

INCLUDEPATH += $$PWD/src
INCLUDEPATH += include/

VPATH = $$PWD/src
SOURCES += main.cpp \
    openglwindow.cpp \
    src/gpuprogram.cpp \
    src/utility.cpp

HEADERS  += \
    openglwindow.h \
    src/main.h \
    src/gpuprogram.h \
    src/utility.h

DISTFILES += \
    data/VS_bloom.glsl \
    data/FS_bloom.glsl

LIBS += -lopengl32
