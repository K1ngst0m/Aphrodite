cmake_minimum_required(VERSION 3.24)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")

project(learnVK)

include(cmake/cflags.cmake)
include(cmake/external.cmake)

include_directories(external)
include_directories(external/glm)
include_directories(external/tinygltf)
include_directories(external/glfw/include)

include_directories(base)

add_subdirectory(base)
add_subdirectory(learnogl2vk)

find_package(xcb REQUIRED)
