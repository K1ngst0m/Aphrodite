project(Sandbox_Others)

file(GLOB SRC_FILES *.cpp glad/src/glad.c)
include_directories(glad/include)

add_executable(${PROJECT_NAME} ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} glfw)
