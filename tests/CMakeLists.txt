include (FetchContent)

enable_testing ()

cpmaddpackage (NAME Catch2 GITHUB_REPOSITORY catchorg/Catch2 GIT_TAG v3.8.0)

file (GLOB TEST_FILES *.cpp)

foreach (TEST_FILE ${TEST_FILES})
    # Extract filename without extension to use as the test target name
    get_filename_component (TEST_TARGET ${TEST_FILE} NAME_WE)
    set (TEST_TARGET "${TEST_TARGET}_t")

    # Create an executable for the test file
    add_executable (${TEST_TARGET} ${TEST_FILE})
    aph_compiler_options (${TEST_TARGET})

    # Link necessary libraries, for instance, Catch2
    target_link_libraries (${TEST_TARGET} PRIVATE aphrodite::engine Catch2::Catch2WithMain)
    target_include_directories (${TEST_TARGET} PRIVATE ${PROJECT_SOURCE_DIR}/engine)
    set_target_properties (
        ${TEST_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${APH_OUTPUT_DIR}/tests"
    )

    # If using CTest, add the test to CTest's test list Assuming each test executable takes no
    # arguments and that the name of the executable is the same as the name of the test.
    add_test (NAME ${TEST_TARGET} COMMAND ${TEST_TARGET})
endforeach ()

add_custom_target (
    runtests
    COMMAND ${CMAKE_CTEST_COMMAND}
    COMMENT "Running all tests..."
)
