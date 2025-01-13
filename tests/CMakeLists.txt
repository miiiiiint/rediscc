include(CTest)
enable_testing()
set(LINK_LIBRARIES rediscc gtest gtest_main)

set(TESTS_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/tests)

file(MAKE_DIRECTORY ${TESTS_OUTPUT_DIR})

file(GLOB_RECURSE TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

find_program(VALGRIND_EXECUTABLE valgrind)

if(VALGRIND_EXECUTABLE)
    message(STATUS "Valgrind found: ${VALGRIND_EXECUTABLE}")
else()
    message(WARNING "Valgrind not found!")
endif()

foreach(TEST_FILE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)

    set(TARGET_NAME "test_${TEST_NAME}")

    add_executable(${TARGET_NAME} ${TEST_FILE})
    add_dependencies(${TARGET_NAME} ${LINK_LIBRARIES})

    target_link_libraries(${TARGET_NAME} ${LINK_LIBRARIES})

    set_target_properties(${TARGET_NAME} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY ${TESTS_OUTPUT_DIR})

    add_test(
        NAME ${TARGET_NAME}
        COMMAND /bin/bash -c "
            ulimit -n 65536 && mkdir -p ${CMAKE_BINARY_DIR}/valgrind &&
            valgrind --leak-check=full --track-origins=yes --error-exitcode=1 
            ${TESTS_OUTPUT_DIR}/${TARGET_NAME} 2>&1 | tee ${CMAKE_BINARY_DIR}/valgrind/valgrind_output_${TEST_NAME}.log;
            exit \${PIPESTATUS[0]}"
    )
endforeach()
