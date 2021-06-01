project(imgui)

file(GLOB src_file *.cpp)

add_library(${PROJECT_NAME} STATIC ${src_file})