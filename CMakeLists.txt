cmake_minimum_required(VERSION 3.24)

project(learnVK)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
set(Vulkan_MIN_VERSION 1.1.70.0)

include(cmake/cflags.cmake)
include(cmake/external.cmake)

include_directories(external)
include_directories(external/glm)
include_directories(external/tinygltf)
include_directories(external/imgui)
include_directories(external/glfw/include)

include_directories(base)

add_subdirectory(base)
add_subdirectory(learnogl2vk)

find_package(Vulkan ${Vulkan_MIN_VERSION} REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(xcb REQUIRED IMPORTED_TARGET xcb)
