cmake_minimum_required(VERSION 3.17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(HazelEngine)

add_definitions(
        -DHZ_PLATFORM_WINDOWS
        -DHZ_ENABLE_ASSERTS
        )

include_directories(
        ${CMAKE_SOURCE_DIR}/Platform
        ${CMAKE_SOURCE_DIR}/Hazel/src/
        ${CMAKE_SOURCE_DIR}/Hazel/src/Hazel
        ${CMAKE_SOURCE_DIR}/Hazel/src/Hazel/Events
        ${CMAKE_SOURCE_DIR}/Hazel/vendor/spdlog/include
        ${CMAKE_SOURCE_DIR}/Hazel/vendor/glfw/include
        )

add_subdirectory(Hazel)
add_subdirectory(Sandbox)