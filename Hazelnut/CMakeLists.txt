project(Editor)

include_directories(../Hazel/3rdparty/entt/include)
include_directories(../Hazel/src)

add_executable(${PROJECT_NAME} src/EditorLayer.cpp src/EditorLayer.h src/HazelnutApp.cpp src/Panels/SceneHierarchyPanel.cpp src/Panels/SceneHierarchyPanel.h)
target_link_libraries(${PROJECT_NAME} Hazel)
