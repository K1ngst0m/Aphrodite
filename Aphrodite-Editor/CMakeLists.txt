project(Aphrodite-Editor)

file(GLOB src_files
        src/*.cpp
        src/Panels/*.cpp
        src/Others/*.cpp
        src/Utils/*.cpp
        )

add_executable(${PROJECT_NAME} ${src_files})

target_link_libraries(${PROJECT_NAME} Aphrodite-Runtime)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets
        )
