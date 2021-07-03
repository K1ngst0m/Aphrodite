project(Aphrodite-Example)

file(GLOB src_files src/*.cpp)

add_executable(${PROJECT_NAME} ${src_files})

target_link_libraries(${PROJECT_NAME} Aphrodite-Runtime)

file(COPY assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
