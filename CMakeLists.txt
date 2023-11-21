cmake_minimum_required(VERSION 3.24)

project(Aphrodite)

# Provide an option to enable testing
option(APH_ENABLE_TESTING "Enable testing for the APH project" ON)
option(APH_ENABLE_TRACING "Enable tracer for the APH project" ON)

set(APH_ROOT_DIR ${CMAKE_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${APH_ROOT_DIR}/cmake")

include(AphDefinitions)
include(AphCompilerOptions)
include(AphExternal)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${APH_OUTPUT_DIR}/debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${APH_OUTPUT_DIR}/release")

add_subdirectory(engine)
add_subdirectory(examples)

if (APH_ENABLE_TESTING)
    add_subdirectory(tests)
endif()
