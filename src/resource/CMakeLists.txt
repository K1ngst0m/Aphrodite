file (GLOB API_RESOURCE_SRC *.cpp)
aph_setup_target (resource ${API_RESOURCE_SRC})

target_link_libraries (
    aph-resource
    PRIVATE aph-filesystem
            aph-common
            aph-api
            aph-math
            aph-global
            tinygltf
            stb
            slang
            spirv-cross-core
)
