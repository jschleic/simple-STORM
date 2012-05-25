project(sifutils)
cmake_minimum_required(VERSION 2.8)
SET(CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH}  ${storm_SOURCE_DIR}/../cmake)

FIND_PACKAGE( Vigra 1.8.0 REQUIRED )
FIND_PACKAGE( HDF5 REQUIRED )

ADD_EXECUTABLE(sif2hdf5 EXCLUDE_FROM_ALL sif2hdf5.cpp)
ADD_EXECUTABLE(sif2tiff EXCLUDE_FROM_ALL sif2tiff.cpp)
ADD_EXECUTABLE(hdf52tiff EXCLUDE_FROM_ALL hdf52tiff.cpp)
ADD_EXECUTABLE(tiff2hdf5 EXCLUDE_FROM_ALL tiff2hdf5.cpp)
ADD_CUSTOM_TARGET(sifutils DEPENDS sif2hdf5 sif2tiff hdf52tiff tiff2hdf5)
TARGET_LINK_LIBRARIES(sif2hdf5 vigraimpex)
TARGET_LINK_LIBRARIES(sif2tiff vigraimpex)
TARGET_LINK_LIBRARIES(hdf52tiff vigraimpex)
TARGET_LINK_LIBRARIES(tiff2hdf5 vigraimpex)


IF(HDF5_FOUND)
	ADD_DEFINITIONS(-DHDF5_FOUND)
ELSE(HDF5_FOUND)
	message(WARNING "Compiling without HDF5. No hdf5-input will be possible")
ENDIF(HDF5_FOUND)

ADD_EXECUTABLE(conv_3d convert.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../storm/myimportinfo.cpp)
INCLUDE_DIRECTORIES (
    ${HDF5_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../storm/
    ${Vigra_INCLUDE_DIRS}
    )
TARGET_LINK_LIBRARIES(conv_3d vigraimpex)
