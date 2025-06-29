#-------------------------------------------------
#
# Project created by QtCreator 2015-06-19T08:43:46
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

#RC_ICONS = ta985m.ico


include( $$_PRO_FILE_PWD_/rtk_version.pri )

CONFIG(debug, debug|release):{
    TARGET = $$sprintf('%1_d', $$TARGET)
    #DEFINES += __DEBUG__
}else:{
    TARGET = $$sprintf('%1', $$TARGET)
}

    RC_FILE     += rc_info.rc
    OTHER_FILES += rc_info.rc \
    ../../Install/version.txt


TEMPLATE = app


CONFIG +=c++11

SOURCES +=      main.cpp\
                mainwindow.cpp \
                3rdparty/libmodbus/modbus.c \
                3rdparty/libmodbus/modbus-tcp.c \
                3rdparty/libmodbus/modbus-rtu.c \
                3rdparty/libmodbus/modbus-data.c \
    dcheckkadr.cpp \
    mas_data.cpp \
    fwritekadr.cpp \
    3rdparty/quazip/zip.c \
    3rdparty/quazip/unzip.c \
    3rdparty/quazip/quazipnewinfo.cpp \
    3rdparty/quazip/quazipfileinfo.cpp \
    3rdparty/quazip/quazipfile.cpp \
    3rdparty/quazip/quazipdir.cpp \
    3rdparty/quazip/quazip.cpp \
    3rdparty/quazip/quaziodevice.cpp \
    3rdparty/quazip/quagzipfile.cpp \
    3rdparty/quazip/quacrc32.cpp \
    3rdparty/quazip/quaadler32.cpp \
    3rdparty/quazip/qioapi.cpp \
    3rdparty/quazip/JlCompress.cpp \
    dmanualloadpzu.cpp


HEADERS  += mainwindow.h \
    dnewustavki.h \
            3rdparty/libmodbus/modbus.h \
            3rdparty/libmodbus/modbus-version.h \
            3rdparty/libmodbus/modbus-tcp.h \
            3rdparty/libmodbus/modbus-tcp-private.h \
            3rdparty/libmodbus/modbus-rtu.h \
            3rdparty/libmodbus/modbus-rtu-private.h \
            3rdparty/libmodbus/modbus-private.h \
            3rdparty/libmodbus/config.h \
    pult.h \
    dcheckkadr.h \
    mas_data.h \
    dmanualreceiver.h \
    3rdparty/quazip/zip.h \
    3rdparty/quazip/unzip.h \
    3rdparty/quazip/quazipnewinfo.h \
    3rdparty/quazip/quazipfileinfo.h \
    3rdparty/quazip/quazipfile.h \
    3rdparty/quazip/quazipdir.h \
    3rdparty/quazip/quazip_global.h \
    3rdparty/quazip/quazip.h \
    3rdparty/quazip/quaziodevice.h \
    3rdparty/quazip/quagzipfile.h \
    3rdparty/quazip/quacrc32.h \
    3rdparty/quazip/quachecksum32.h \
    3rdparty/quazip/quaadler32.h \
    3rdparty/quazip/JlCompress.h \
    3rdparty/quazip/ioapi.h \
    3rdparty/quazip/crypt.h \
    dmanualloadpzu.h

FORMS    +=     mainwindow.ui \
                dadddevice.ui \
                dnewustavki.ui \
    dcheckkadr.ui \
    fwritekadr.ui \
    dviewerror.ui \
    dviewustavki.ui \
    dmanualloadpzu.ui


INCLUDEPATH += 3rdparty/libmodbus

INCLUDEPATH += 3rdparty/quazip
DEPENDPATH += 3rdparty/quazip

INCLUDEPATH += c:/Qt/Qt5.1.0n/5.1.0/Src/qtbase/src/3rdparty/zlib

DEFINES += QUAZIP_STATIC
DEFINES += QUAZIP_BUILD

win32:DEFINES   += _TTY_WIN_ WINVER=0x0501
win32:LIBS      += -lsetupapi -lwsock32 -lws2_32 -lz

