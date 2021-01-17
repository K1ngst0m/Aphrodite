cmake_minimum_required(VERSION 3.17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
project(Hazel)

file(GLOB SRC_FILES
        src/*.cpp
        src/Hazel/*.cpp
        )

message(STATUS "SRC_FILES = ${SRC_FILES}")

add_definitions(-DHZ_BUILD_DLL)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
target_link_libraries(${PROJECT_NAME})