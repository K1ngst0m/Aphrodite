cmake_minimum_required(VERSION 3.24)

project(Aphrodite)

# Provide an option to enable testing
option(APH_ENABLE_TESTING "Enable testing" OFF)
option(APH_ENABLE_TRACING "Enable tracer" OFF)
option(APH_ENABLE_TSAN "Enable thread sanitizer" OFF)
option(APH_ENABLE_ASAN "Enable address sanitizer" OFF)
option(APH_ENABLE_MSAN "Enable memory sanitizer" OFF)

SET(APH_WSI_BACKEND "Auto" CACHE STRING "WSI backend (possible values: Auto, GLFW, SDL2)")
SET_PROPERTY(CACHE APH_WSI_BACKEND PROPERTY STRINGS Auto GLFW SDL2)

set(APH_ROOT_DIR ${CMAKE_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${APH_ROOT_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${APH_ROOT_DIR}/cmake/modules")

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
