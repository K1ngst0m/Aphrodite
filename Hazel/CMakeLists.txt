project(Hazel)

# cpp source file
file(GLOB SRC_FILES
        src/*.cpp
        src/Hazel/*.cpp
        src/Hazel/Events/*.cpp
        src/Hazel/ImGui/*.cpp
        src/Platform/Windows/*.cpp
        src/Platform/OpenGL/*.cpp
        vendor/imgui/backends/imgui_impl_glfw.cpp
        vendor/imgui/backends/imgui_impl_opengl3.cpp
        )

# pre definitions
add_definitions(-DHZ_PLATFORM_WINDOWS -DHZ_BUILD_DLL)

# build project
add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} glfw opengl32 imgui spdlog pthread glad glm)
target_precompile_headers(${PROJECT_NAME} PUBLIC src/hzpch.h)

# submodule
add_subdirectory(vendor/glfw)
add_subdirectory(vendor/spdlog)
add_subdirectory(vendor/glad)
add_subdirectory(vendor/imgui)
add_subdirectory(vendor/glm)
