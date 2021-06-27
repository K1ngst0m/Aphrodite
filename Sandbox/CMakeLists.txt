project(Sandbox)

include_directories(3rdparty/glm/)
include_directories(3rdparty/entt/include)
include_directories(../Hazel/src)

add_executable(${PROJECT_NAME} src/SandboxApp.cpp src/Sandbox2D.cpp src/Sandbox2D.h src/ExampleLayer.cpp src/ExampleLayer.h)
target_link_libraries(${PROJECT_NAME} Hazel)
