cmake_minimum_required(VERSION 3.24)

project(Aphrodite)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")

include(AphCompilerOptions)
include(AphExternal)

add_subdirectory(engine)
add_subdirectory(examples)

find_package(PkgConfig REQUIRED)
pkg_check_modules(xcb REQUIRED IMPORTED_TARGET xcb)
