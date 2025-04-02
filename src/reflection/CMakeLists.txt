file (GLOB API_REFLECTION_SRC *.cpp)
aph_setup_target (reflection ${API_REFLECTION_SRC})

target_link_libraries (
    aph-reflection
    PRIVATE aph-filesystem
            aph-common
            aph-api
            aph-global
            tinygltf
            spirv-cross-core
)
