cmake_minimum_required(VERSION 3.24)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")

project(learnVK)

include(cmake/cflags.cmake)
include(cmake/findXCB.cmake)

include_directories(external)
include_directories(external/glm)
include_directories(external/tinygltf)

include_directories(base)

find_library(glfw_lib NAMES glfw)
if (NOT glfw_lib)
    message(FATAL_ERROR "Could not find GLFW library!")
endif()

find_library(vulkan_lib NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/libs/vulkan" REQUIRED)
if (vulkan_lib)
    message(STATUS "Using bundled Vulkan library version")
else()
    message(FATAL_ERROR "Could not find Vulkan library!")
endif()

add_subdirectory(base)
add_subdirectory(learnogl2vk)
