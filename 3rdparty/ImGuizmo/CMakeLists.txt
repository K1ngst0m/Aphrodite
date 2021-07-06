project(imGuizmo)

include_directories(../imgui)

#file(GLOB src_file ${PROJECT_SOURCE_DIR}/*.cpp)

add_library(${PROJECT_NAME} STATIC ImGuizmo.cpp)