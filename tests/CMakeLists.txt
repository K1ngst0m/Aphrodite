include (FetchContent)

enable_testing ()

cpmaddpackage (NAME Catch2 GITHUB_REPOSITORY catchorg/Catch2 GIT_TAG v3.8.0)

file (GLOB TEST_FILES *.cpp)

# Initialize a list to collect all test targets
set (
    APH_TEST_TARGETS
)

foreach (TEST_FILE ${TEST_FILES})
    # Extract filename without extension to use as the test target name
    get_filename_component (TEST_TARGET ${TEST_FILE} NAME_WE)
    set (TEST_TARGET "${TEST_TARGET}_t")

    # Create an executable for the test file
    add_executable (${TEST_TARGET} ${TEST_FILE})
    aph_compiler_options (${TEST_TARGET})

    # Link necessary libraries, for instance, Catch2
    target_link_libraries (${TEST_TARGET} PRIVATE aphrodite::all Catch2::Catch2WithMain)
    target_include_directories (${TEST_TARGET} PRIVATE ${PROJECT_SOURCE_DIR}/engine)
    set_target_properties (
        ${TEST_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${APH_OUTPUT_DIR}/tests"
    )

    # If using CTest, add the test to CTest's test list Assuming each test executable takes no
    # arguments and that the name of the executable is the same as the name of the test.
    add_test (NAME ${TEST_TARGET} COMMAND ${TEST_TARGET})

    # Append to the list of test targets
    list (APPEND APH_TEST_TARGETS ${TEST_TARGET})
endforeach ()

# Make sure we enable testing at the project level
enable_testing ()

# For each test you add, ensure it's registered with CTest Example: if(TARGET test_example)
# list(APPEND APH_TEST_TARGETS test_example) add_test(NAME test_example COMMAND test_example)
# endif()

# Custom target that depends on all tests
add_custom_target (
    aphrodite-tests
    DEPENDS ${APH_TEST_TARGETS}
    COMMENT "Building all tests..."
)

# Second target that runs all tests
add_custom_target (
    run-aphrodite-tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS aphrodite-tests
    COMMENT "Running all tests..."
)
