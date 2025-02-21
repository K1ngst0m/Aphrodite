file(GLOB API_RESOURCE_SRC ${APH_ENGINE_RESOURCE_DIR}/*.cpp)
aph_setup_target(resource ${API_RESOURCE_SRC})

target_link_libraries(aph-resource PRIVATE aph-filesystem aph-common aph-api tinygltf stb slang spirv-cross-core)
