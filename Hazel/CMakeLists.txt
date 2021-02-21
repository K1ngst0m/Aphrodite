project(Hazel)

file(GLOB SRC_FILES
        src/*.cpp
        src/Hazel/*.cpp
        src/Hazel/Events/*.cpp
        src/Hazel/ImGui/*.cpp
        src/Platform/*.cpp
        src/Platform/OpenGL/*.cpp
        )

add_definitions(
        -DHZ_PLATFORM_WINDOWS
        -DHZ_BUILD_DLL
        )

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_link_libraries(${PROJECT_NAME} glfw opengl32 imgui spdlog pthread glad)

target_precompile_headers(${PROJECT_NAME} PUBLIC src/hzpch.h)

add_subdirectory(vendor/glfw)
add_subdirectory(vendor/spdlog)
add_subdirectory(vendor/glad)
add_subdirectory(vendor/imgui)
