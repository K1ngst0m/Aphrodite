set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(external/glfw)

find_library(vulkan_lib NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/libs/vulkan" REQUIRED)
if (vulkan_lib)
    message(STATUS "Using bundled Vulkan library version")
else()
    message(FATAL_ERROR "Could not find Vulkan library!")
endif()
