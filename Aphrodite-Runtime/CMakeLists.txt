project(Aphrodite-Runtime)

add_definitions(-DAPH_ENABLE_ASSERTS -DAPH_DEBUG)

file(GLOB src_root src/pch.cpp)
file(GLOB src_core src/Aphrodite/Core/*.cpp)
file(GLOB src_debug src/Aphrodite/Debug/*.cpp)
file(GLOB src_physics src/Aphrodite/Physics/*.cpp)
file(GLOB src_input src/Aphrodite/Input/*.cpp)
file(GLOB src_events src/Aphrodite/Events/*.cpp)
file(GLOB src_fonts src/Aphrodite/Fonts/*.cpp)
file(GLOB src_imgui src/Aphrodite/UI/*.cpp src/Aphrodite/UI/Utilities/*.cpp)
file(GLOB src_math src/Aphrodite/Math/*.cpp)
file(GLOB src_renderer src/Aphrodite/Renderer/*.cpp)
file(GLOB src_scene src/Aphrodite/Scene/*.cpp)
file(GLOB src_utils src/Aphrodite/Utils/*.cpp)
file(GLOB src_platform_linux src/Platform/Linux/*.cpp)
file(GLOB src_platform_opengl src/Platform/OpenGL/*.cpp)

add_library(${PROJECT_NAME} STATIC
        ${src_root}
        ${src_physics}
        ${src_input}
        ${src_core}
        ${src_debug}
        ${src_events}
        ${src_fonts}
        ${src_imgui}
        ${src_math}
        ${src_renderer}
        ${src_scene}
        ${src_utils}
        ${src_platform_linux}
        ${src_platform_opengl})

target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)

target_link_libraries(${PROJECT_NAME} PUBLIC
        ${vulkan}
        ${opengl_lib}
        ${ui_lib}
        ${other_lib}
        )