project(Editor)

include_directories(../Hazel/src)

add_library(${PROJECT_NAME} src/EditorLayer.cpp src/EditorLayer.h src/HazelnutApp.cpp)
target_link_libraries(${PROJECT_NAME} Hazel)
