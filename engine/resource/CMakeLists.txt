file(GLOB API_RESOURCE_SRC ${APH_ENGINE_RESOURCE_DIR}/*.cpp)
add_library(resource STATIC ${API_RESOURCE_SRC})
aph_compiler_options(resource)
target_include_directories(resource PRIVATE
  ${APH_ENGINE_DIR}
  ${APH_EXTERNAL_DIR}/tinyktx
  ${APH_EXTERNAL_DIR}/stb/include
  ${tinygltf_SOURCE_DIR}

  ${slang_SOURCE_DIR}
)

target_link_libraries(resource PRIVATE aph_filesystem common api)
target_link_libraries(resource PRIVATE
  ${slang_SOURCE_DIR}/bin/linux-x64/release/libslang.so
)
