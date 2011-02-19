ADD_EXECUTABLE(sif2hdf5 EXCLUDE_FROM_ALL sif2hdf5.cpp)
ADD_DEPENDENCIES(experiments sif2hdf5)
TARGET_LINK_LIBRARIES(sif2hdf5 vigraimpex)

