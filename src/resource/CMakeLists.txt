file (GLOB API_RESOURCE_SRC
    *.cpp
    image/*.cpp
    buffer/*.cpp
    geometry/*.cpp
    shader/*.cpp
    material/*.cpp
)
aph_setup_target (resource ${API_RESOURCE_SRC})

target_link_libraries (
    aph-resource
    PRIVATE aph-filesystem
            aph-common
            aph-api
            aph-math
            aph-global
            aph-geometry
            aph-reflection
            aph-material
            tinygltf
            stb
            ktx
            slang
            meshoptimizer
)

