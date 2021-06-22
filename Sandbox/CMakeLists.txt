project(Sandbox)

include_directories(3rdparty/glm/)
include_directories(../Hazel/src)

add_executable(${PROJECT_NAME} src/SandboxApp.cpp src/Sandbox2D.cpp src/Sandbox2D.h)
target_link_libraries(${PROJECT_NAME} Hazel)
