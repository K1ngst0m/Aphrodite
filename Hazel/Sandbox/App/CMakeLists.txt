project(SandBox)
file(GLOB SRC_FILES Sandbox.cpp)
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} Hazel)
