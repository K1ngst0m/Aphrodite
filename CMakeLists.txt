cmake_minimum_required(VERSION 3.24)

project(Aphrodite)

set(APH_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)
set(APH_ROOT_DIR ${CMAKE_SOURCE_DIR})
set(APH_EXAMPLES_DIR ${APH_ROOT_DIR}/examples)
set(APH_ENGINE_DIR ${APH_ROOT_DIR}/engine)
set(APH_EXTERNAL_DIR ${APH_ROOT_DIR}/external)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${APH_ROOT_DIR}/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${APH_OUTPUT_DIR}")

include(AphCompilerOptions)
include(AphExternal)

add_subdirectory(engine)
add_subdirectory(examples)
