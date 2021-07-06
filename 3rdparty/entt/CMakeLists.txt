project(entt)

include_directories(include)

add_library(${PROJECT_NAME} STATIC include/entt.hpp entt.cpp)