ADD_EXECUTABLE(test_ EXCLUDE_FROM_ALL testFix.cpp)
ADD_DEPENDENCIES(experiments test_)
TARGET_LINK_LIBRARIES(test_ vigraimpex)

