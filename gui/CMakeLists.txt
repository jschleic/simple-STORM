project(storm-gui)
cmake_minimum_required(VERSION 2.8)

find_package(Qt4 COMPONENTS QtCore QtSql QtGui QtNetwork REQUIRED)
FIND_PACKAGE(Vigra REQUIRED)

include(${QT_USE_FILE} )


if(QT_PHONON_FOUND)
    add_definitions(-DWITH_PHONON)
    include_directories(${QT_PHONON_INCLUDE_DIR})
endif(QT_PHONON_FOUND)
set(SRCS
    main.cpp
    mainwindow.cpp
    mainview.cpp
    maincontroller.cpp
    stormparamsdialog.cpp
    settingsdialog.cpp
    filenamelineedit.cpp
    stormmodel.cpp
    stormprocessor.cpp
    config.cpp
    ../storm/myimportinfo.cpp
    ../storm/fftfilter.cpp
    ../storm/util.cpp
    )
set(MOC_H
    mainwindow.h
    mainview.h
    maincontroller.h
    stormparamsdialog.h
    settingsdialog.h
    filenamelineedit.h
    stormmodel.h
    )
SET(MY_UIS mainwindow.ui mainview.ui stormparamsdialog.ui settingsdialog.ui)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../storm ${Vigra_INCLUDE_DIRS})
qt4_wrap_ui(SRCS_CXX ${MY_UIS})
qt4_wrap_cpp(SRCS_CXX ${MOC_H})
add_executable(storm-gui WIN32 ${SRCS} ${SRCS_CXX})

IF(UNIX)
    target_link_libraries(storm-gui ${QT_LIBRARIES} ${QT_PHONON_LIBRARY} vigraimpex fftw3 fftw3f)
ELSEIF(MINGW)
    # FindQt4 has problems with static linking, so we have to unset QT_DLL
    set_target_properties(storm-gui PROPERTIES COMPILE_FLAGS "-UQT_DLL")
    # implicitly needed by Qt but cmake does not find them when cross-compiling using mingw
    # ?because of static linking?
    FIND_PACKAGE(JPEG REQUIRED)
    FIND_PACKAGE(PNG REQUIRED)
    FIND_PACKAGE(TIFF REQUIRED)

    add_definitions(-DVIGRA_STATIC_LIB)
    target_link_libraries(storm-gui ${QT_LIBRARIES} -lvigraimpex fftw3 fftw3f ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${TIFF_LIBRARIES} -limm32 -lwinmm -lmng -llcms -lssl -lcrypto -lws2_32 -lIlmImf -lz -lImath -lHalf -lIex -lIlmThread qcncodecs jpcodecs twcodecs krcodecs)
ENDIF()

set(BIN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/bin})
set(DATA_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})
install(TARGETS storm-gui DESTINATION ${BIN_INSTALL_DIR})
