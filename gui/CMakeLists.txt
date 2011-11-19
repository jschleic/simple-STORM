project(storm-gui)
cmake_minimum_required(VERSION 2.8)
SET(CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH}  ${storm-gui_SOURCE_DIR}/cmake)

find_package(Qt4 4.6 COMPONENTS QtCore QtGui REQUIRED)
FIND_PACKAGE(Vigra 1.8.0 REQUIRED)
FIND_PACKAGE(FFTW REQUIRED)

include(${QT_USE_FILE} )

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
    previewtimer.cpp
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
    previewtimer.h
    )
SET(MY_UIS mainwindow.ui mainview.ui stormparamsdialog.ui settingsdialog.ui)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../storm ${Vigra_INCLUDE_DIRS} ${FFTW_INCLUDE_DIRS})
qt4_wrap_ui(SRCS_CXX ${MY_UIS})
qt4_wrap_cpp(SRCS_CXX ${MOC_H})
add_executable(storm-gui ${SRCS} ${SRCS_CXX})

target_link_libraries(storm-gui ${QT_LIBRARIES} vigraimpex ${FFTW_LIBRARIES})


set(BIN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/bin})
set(DATA_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})
install(TARGETS storm-gui DESTINATION ${BIN_INSTALL_DIR})
