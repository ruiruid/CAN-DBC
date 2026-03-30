QT += core websockets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Protobuf配置
INCLUDEPATH += C:/protobuf-cpp-3.21.12/protobuf-3.21.12/src
LIBS += -LC:/protobuf-cpp-3.21.12/protobuf-3.21.12/build -lprotobuf

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    server.cpp \
    can_message.pb.cc

HEADERS += \
    mainwindow.h \
    server.h \
    can_message.pb.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
