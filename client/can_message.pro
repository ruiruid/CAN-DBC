QT       += core gui network websockets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Protobuf 配置
INCLUDEPATH += C:/protobuf-cpp-3.21.12/protobuf-3.21.12/src
LIBS += -LC:/protobuf-cpp-3.21.12/protobuf-3.21.12/build -lprotobuf

SOURCES += \
    can_message.pb.cc \
    client.cpp \
    dbcdialog.cpp \
    dbcparser.cpp \
    main.cpp \
    mainwindow.cpp \
    signaldialog.cpp

HEADERS += \
    can_message.pb.h \
    client.h \
    dbcdialog.h \
    dbcparser.h \
    mainwindow.h \
    signaldialog.h

FORMS += \
    client.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
