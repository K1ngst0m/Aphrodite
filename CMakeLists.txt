cmake_minimum_required(VERSION 3.24)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")

project(learnVK)

include(cmake/cflags.cmake)
include_directories(base)

add_subdirectory(base)
add_subdirectory(learnogl2vk)
