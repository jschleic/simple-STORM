 CONFIG += qt

 FORMS += mainview.ui
 FORMS += mainwindow.ui
 FORMS += stormparamsdialog.ui
 FORMS += settingsdialog.ui
 LIBS += -lvigraimpex -lfftw3f -lfftw3 `'i686-pc-mingw32-pkg-config' OpenEXR --cflags --libs` -ltiff -lpng -ljpeg -lz
 DEFINES += VIGRA_STATIC_LIB

 INCLUDEPATH += ../storm
 HEADERS += mainwindow.h
 HEADERS +=    mainview.h
 HEADERS +=    maincontroller.h
 HEADERS +=    stormparamsdialog.h
 HEADERS +=    filenamelineedit.h
 HEADERS +=    stormmodel.h
 HEADERS +=    settingsdialog.h
 HEADERS +=    previewtimer.h
    
 SOURCES +=   main.cpp
 SOURCES +=     mainwindow.cpp
 SOURCES +=     mainview.cpp
 SOURCES +=     maincontroller.cpp
 SOURCES +=     stormparamsdialog.cpp
 SOURCES +=     filenamelineedit.cpp
 SOURCES +=     stormmodel.cpp
 SOURCES +=     stormprocessor.cpp
 SOURCES +=     settingsdialog.cpp
 SOURCES +=     previewtimer.cpp
 SOURCES +=     config.cpp
 SOURCES +=     ../storm/myimportinfo.cpp
 SOURCES +=     ../storm/fftfilter.cpp
 SOURCES +=     ../storm/util.cpp

 TARGET = storm-gui
