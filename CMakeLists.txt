cmake_minimum_required(VERSION 3.18)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Libs)

set(3rdparty_dir ${CMAKE_SOURCE_DIR}/3rdparty)
set(runtime_src_dir ${CMAKE_SOURCE_DIR}/Aphrodite-Runtime)
set(editor_src_dir ${CMAKE_SOURCE_DIR}/Aphrodite-Editor)

# runtime
include_directories(${runtime_src_dir}/include)
include_directories(${runtime_src_dir}/src)

# 3rdparty
set(BUILD_SHARED_LIBS ON)
add_subdirectory(${3rdparty_dir}/glad)
add_subdirectory(${3rdparty_dir}/entt)
add_subdirectory(${3rdparty_dir}/spdlog)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(${3rdparty_dir}/glfw)
set(IMGUI_BACKEND_OPENGL3 ON)
set(IMGUI_BACKEND_GLFW ON)
add_subdirectory(${3rdparty_dir}/imgui)
add_subdirectory(${3rdparty_dir}/ImGuizmo)
set(BOX2D_BUILD_UNIT_TESTS OFF)
set(BOX2D_BUILD_TESTBED OFF)
add_subdirectory(${3rdparty_dir}/box2d)
# TODO: assimp
# set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
# set(ASSIMP_BUILD_TESTS OFF)
# set(ASSIMP_BUILD_ZLIB ON)
# add_subdirectory(${3rdparty_dir}/assimp)
set(YAML_CPP_BUILD_TOOLS OFF)
add_subdirectory(${3rdparty_dir}/yaml-cpp)
set(SPIRV_CROSS_ENABLE_TESTS OFF)
set(SPIRV_CROSS_ENABLE_HLSL OFF)
set(SPIRV_CROSS_ENABLE_MSL OFF)
set(SPIRV_CROSS_CLI OFF)
add_subdirectory(${3rdparty_dir}/spirv-cross)

include_directories(${3rdparty_dir}/stb)
include_directories(${3rdparty_dir}/glad/include)
include_directories(${3rdparty_dir}/entt/include)
include_directories(${3rdparty_dir}/spirv-cross)
# TODO: assimp
# include_directories(${3rdparty_dir}/assimp/include)
include_directories(${3rdparty_dir}/imgui)
include_directories(${3rdparty_dir}/glm)
include_directories(${3rdparty_dir}/yaml-cpp)
include_directories(${3rdparty_dir}/ImGuizmo)

# lib
set(opengl_lib glad glfw GL)
set(vulkan shaderc_shared spirv-cross-core spirv-cross-glsl SPIRV-Tools)
set(ui_lib imgui imGuizmo)
set(other_lib box2d spdlog::spdlog yaml-cpp entt assimp)
link_directories(${CMAKE_BINARY_DIR}/Libs)

project(Aphrodite)

add_subdirectory(Aphrodite-Editor)
add_subdirectory(Aphrodite-Runtime)
# add_subdirectory(Aphrodite-Example)

add_compile_options(-Wall -Wextra -Werror)
