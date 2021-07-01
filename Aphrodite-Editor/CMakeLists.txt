project(Aphrodite-Editor)

include_directories(${3rdparty_dir}/imgui)
include_directories(${3rdparty_dir}/ImGuizmo)
include_directories(${3rdparty_dir}/entt/include)
include_directories(${runtime_src_dir}/src)

add_executable(${PROJECT_NAME}
        src/EditorLayer.cpp
        src/EditorLayer.h
        src/Editor.cpp
        src/Panels/SceneHierarchyPanel.cpp
        src/Panels/SceneHierarchyPanel.h
        ${3rdparty_dir}/ImGuizmo/ImGuizmo.cpp
        src/Panels/ContentBrowserPanel.cpp src/Panels/ContentBrowserPanel.h)

target_link_libraries(${PROJECT_NAME} Aphrodite-Runtime)

file(COPY assets
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
