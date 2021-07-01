project(Aphrodite-Editor)


add_executable(${PROJECT_NAME}
        src/EditorLayer.cpp
        src/EditorLayer.h
        src/Editor.cpp
        src/Panels/SceneHierarchyPanel.cpp
        src/Panels/SceneHierarchyPanel.h
        src/Panels/ContentBrowserPanel.cpp src/Panels/ContentBrowserPanel.h)

target_link_libraries(${PROJECT_NAME} Aphrodite-Runtime)

file(COPY assets
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
