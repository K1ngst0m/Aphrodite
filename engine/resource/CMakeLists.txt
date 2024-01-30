file(GLOB API_RESOURCE_SRC ${APH_ENGINE_RESOURCE_DIR}/*.cpp)
add_library(resource STATIC ${API_RESOURCE_SRC})
aph_compiler_options(resource)
target_include_directories(resource PRIVATE
  ${APH_ENGINE_DIR}
)

target_link_libraries(resource PRIVATE aph_filesystem common api tinygltf stb)
target_link_libraries(resource PRIVATE slang)
