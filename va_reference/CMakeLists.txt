ADD_EXECUTABLE(sif2tiff EXCLUDE_FROM_ALL sif2tiff.cpp)
#ADD_EXECUTABLE(tts EXCLUDE_FROM_ALL testSpline.cpp)
ADD_DEPENDENCIES(experiments sif2tiff)
TARGET_LINK_LIBRARIES(sif2tiff vigraimpex)

