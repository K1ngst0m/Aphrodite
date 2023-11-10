file(GLOB API_RESOURCE_SRC ${APH_ENGINE_RESOURCE_DIR}/*.cpp)
add_library(resource STATIC ${API_RESOURCE_SRC})
aph_compiler_options(resource)
target_include_directories(resource PRIVATE
  ${APH_ENGINE_DIR}
  ${APH_EXTERNAL_DIR}/volk
  ${APH_EXTERNAL_DIR}/slang
  ${APH_EXTERNAL_DIR}/tinyktx
  ${APH_EXTERNAL_DIR}/stb/include
  ${APH_EXTERNAL_DIR}/tinygltf
  ${APH_EXTERNAL_DIR}/unordered_dense/include
)

target_link_libraries(resource PRIVATE slang aph_filesystem)
