QT += widgets serialport printsupport

TARGET = terminal
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    settingsdialog.cpp \
    console.cpp \
    graphwidg.cpp \
    qcustomplot.cpp

HEADERS += \
    mainwindow.h \
    settingsdialog.h \
    console.h \
    graphwidg.h \
    qcustomplot.h


FORMS += \
    mainwindow.ui \
    settingsdialog.ui \
    graphwidg.ui

RESOURCES += \
    terminal.qrc
