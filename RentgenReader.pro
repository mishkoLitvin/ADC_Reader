#-------------------------------------------------
#
# Project created by QtCreator 2018-12-16T15:29:07
#
#-------------------------------------------------

QT       += charts qml quick core serialport

TARGET = RentgenReader

SOURCES += \
        main.cpp \
    applicationcontroller.cpp

HEADERS += \
    applicationcontroller.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    main.qml \
    my_components/Dial.qml \
    my_components/ScopeView.qml

RESOURCES += \
    res.qrc
