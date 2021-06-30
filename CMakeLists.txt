cmake_minimum_required(VERSION 3.18)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Libs)

set(3rdparty_dir ${CMAKE_SOURCE_DIR}/Aphrodite-Runtime/3rdparty)
set(runtime_src_dir ${CMAKE_SOURCE_DIR}/Aphrodite-Runtime)
set(editor_src_dir ${CMAKE_SOURCE_DIR}/Aphrodite-Editor)

link_directories(${CMAKE_BINARY_DIR}/Libs)

project(Aphrodite)

add_subdirectory(Aphrodite-Editor)
#add_subdirectory(Sandbox)
add_subdirectory(Aphrodite-Runtime)
