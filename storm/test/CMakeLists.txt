INCLUDE(VigraAddTest)

SET(RUN_STORM_TEST "${CMAKE_CURRENT_BINARY_DIR}/run_testCase.sh")
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/run_test.sh.in
              ${RUN_STORM_TEST}
              @ONLY)
EXECUTE_PROCESS(COMMAND chmod u+x ${RUN_STORM_TEST} OUTPUT_QUIET ERROR_QUIET)
VIGRA_COPY_TEST_DATA(testSif_4_16_30001.sif testCoords.txt testReference.png)

#ADD_DEPENDENCIES(storm test_storm)

