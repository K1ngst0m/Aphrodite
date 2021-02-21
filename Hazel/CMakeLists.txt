project(Hazel)

file(GLOB SRC_FILES
        src/*.cpp
        src/Hazel/*.cpp
        src/Events/*.cpp
        Platform/*.cpp
        )

add_definitions(-DHZ_BUILD_DLL)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_link_libraries(${PROJECT_NAME} glfw opengl32 spdlog pthread)

target_precompile_headers(${PROJECT_NAME} PUBLIC src/hzpch.h)

add_subdirectory(vendor/glfw)
add_subdirectory(vendor/spdlog)
