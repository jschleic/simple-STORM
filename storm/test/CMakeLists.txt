
MACRO(COPY_TEST_DATA)
    FOREACH(test_data ${ARGN})
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${test_data}
                       ${CMAKE_CURRENT_BINARY_DIR}/${test_data}
                       COPYONLY)
    ENDFOREACH(test_data)
ENDMACRO(COPY_TEST_DATA)


SET(RUN_STORM_TEST "${CMAKE_CURRENT_BINARY_DIR}/run_testCase.sh")
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/run_test.sh.in
              ${RUN_STORM_TEST}
              @ONLY)
EXECUTE_PROCESS(COMMAND chmod u+x ${RUN_STORM_TEST} OUTPUT_QUIET ERROR_QUIET)
COPY_TEST_DATA(testSif_4_16_30001.sif testCoords.txt testReference.png)

