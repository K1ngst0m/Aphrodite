cmake_minimum_required(VERSION 3.17)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
project(Hazel)

file(GLOB SRC_FILES
        src/*.cpp
        src/Hazel/*.cpp
        src/Events/*.cpp
        )

add_definitions(-DHZ_BUILD_DLL)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
target_precompile_headers(${PROJECT_NAME} PUBLIC src/hzpch.h)
target_link_libraries(${PROJECT_NAME})

add_subdirectory(test)