INCLUDEPATH += . /usr/include/sigc++-2.0 /usr/lib/sigc++-2.0/include
TEMPLATE = app
TARGET = qpar2
target.path = /usr/bin
INSTALLS += target
OBJECTS_DIR = tmp
CONFIG += qt warn_on
unix:LIBS += -lpar2
HEADERS += mainwindow.h
FORMS += mainwindow.ui
SOURCES += mainwindow.cpp main.cpp
