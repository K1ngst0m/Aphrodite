project(Aphrodite-Example)


add_executable(${PROJECT_NAME} src/ExampleLayer.cpp src/ExampleLayer.h src/Sandbox2D.cpp src/Sandbox2D.h src/SandboxApp.cpp)

target_link_libraries(${PROJECT_NAME} Aphrodite-Runtime)

file(COPY assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
