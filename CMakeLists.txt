cmake_minimum_required(VERSION 3.17)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
project(HazelEngine)
add_definitions(-DHZ_PLATFORM_WINDOWS)
include_directories(
        ${CMAKE_SOURCE_DIR}/Hazel/include
        ${CMAKE_SOURCE_DIR}/Hazel/vendor/spdlog/include
        )
add_subdirectory(Hazel)
add_subdirectory(SandBox)
