project(Sandbox)

include_directories(3rdparty/glm/)
include_directories(../Hazel/src)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} Hazel)
