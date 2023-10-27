cmake_minimum_required(VERSION 3.24)

project(Aphrodite)

# Provide an option to enable testing
option(APH_ENABLE_TESTING "Enable testing for the APH project" OFF)

set(APH_ROOT_DIR ${CMAKE_SOURCE_DIR})
set(APH_EXAMPLES_DIR ${APH_ROOT_DIR}/examples)
set(APH_ENGINE_DIR ${APH_ROOT_DIR}/engine)
set(APH_EXTERNAL_DIR ${APH_ROOT_DIR}/external)
set(APH_OUTPUT_DIR ${APH_ROOT_DIR}/bin)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${APH_ROOT_DIR}/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${APH_OUTPUT_DIR}")

include(AphCompilerOptions)
include(AphExternal)

add_subdirectory(engine)
add_subdirectory(examples)

if (APH_ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
