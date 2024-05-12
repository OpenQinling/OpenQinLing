QT       += core gui gui-private

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    appenddevicewindow.cpp \
    appinfowindow.cpp \
    datasturct.cpp \
    deviceinfowindow.cpp \
    devicemode.cpp \
    executbleimage.cpp \
    initromwindow.cpp \
    main.cpp \
    mainwindow.cpp \
    socdeviceconfig.cpp \
    systemdependfunction.cpp \
    toolfunction.cpp \
    vritualcpu.cpp

HEADERS += \
    appenddevicewindow.h \
    appinfowindow.h \
    datasturct.h \
    deviceinfowindow.h \
    devicemode.h \
    executbleimage.h \
    initromwindow.h \
    mainwindow.h \
    socdeviceconfig.h \
    systemdependfunction.h \
    toolfunction.h \
    vritualcpu.h

FORMS += \
    appenddevicewindow.ui \
    appinfowindow.ui \
    deviceinfowindow.ui \
    initromwindow.ui \
    mainwindow.ui

msvc{
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
}

win32{
    RC_ICONS = AppICON.ico
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    ResFile.qrc
