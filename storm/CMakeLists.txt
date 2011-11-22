FIND_PACKAGE( OpenMP )

IF(HDF5_FOUND)
	ADD_DEFINITIONS(-DHDF5_FOUND)
ELSE(HDF5_FOUND)
	message(WARNING "Compiling without HDF5. No hdf5-input will be possible")
ENDIF(HDF5_FOUND)

IF(CMAKE_COMPILER_IS_GNUCXX)
	ADD_EXECUTABLE(storm EXCLUDE_FROM_ALL storm.cpp program_options_getopt.cpp myimportinfo.cpp util.cpp)
	ADD_EXECUTABLE(wienerfilter EXCLUDE_FROM_ALL wienerfilter.cpp program_options_getopt.cpp myimportinfo.cpp util.cpp)
ELSE(CMAKE_COMPILER_IS_GNUCXX)
	ADD_DEFINITIONS(-DEMULATE_GETOPT)
	ADD_EXECUTABLE(storm EXCLUDE_FROM_ALL storm.cpp getoptMSVC.c program_options_getopt.cpp myimportinfo.cpp util.cpp)
	ADD_EXECUTABLE(wienerfilter EXCLUDE_FROM_ALL wienerfilter.cpp getoptMSVC.c program_options_getopt.cpp myimportinfo.cpp util.cpp)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

ADD_DEPENDENCIES(experiments storm wienerfilter)

IF(OPENMP_FOUND)
	SET_TARGET_PROPERTIES(storm PROPERTIES COMPILE_FLAGS ${OpenMP_CXX_FLAGS})
	SET_TARGET_PROPERTIES(storm PROPERTIES LINK_FLAGS ${OpenMP_CXX_FLAGS})
	SET_TARGET_PROPERTIES(wienerfilter PROPERTIES COMPILE_FLAGS ${OpenMP_CXX_FLAGS})
	SET_TARGET_PROPERTIES(wienerfilter PROPERTIES LINK_FLAGS ${OpenMP_CXX_FLAGS})
ENDIF(OPENMP_FOUND)

TARGET_LINK_LIBRARIES(storm vigraimpex ${FFTW3_LIBRARIES} ${FFTW3F_LIBRARIES})
TARGET_LINK_LIBRARIES(wienerfilter vigraimpex ${FFTW3_LIBRARIES} ${FFTW3F_LIBRARIES})
INCLUDE_DIRECTORIES (${FFTW3_INCLUDE_DIR})

ADD_SUBDIRECTORY(test)
