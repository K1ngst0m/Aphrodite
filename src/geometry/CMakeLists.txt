file (GLOB API_GEOMETRY_SRC *.cpp)
aph_setup_target (geometry ${API_GEOMETRY_SRC})

target_link_libraries (
    aph-geometry
    PRIVATE aph-filesystem
            aph-common
            aph-api
            aph-math
            aph-global
            aph-reflection
            tinygltf
            stb
            slang
            meshoptimizer
)
