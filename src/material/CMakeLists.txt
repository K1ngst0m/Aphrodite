file (GLOB API_MATERIAL_SRC *.cpp)
aph_setup_target (material ${API_MATERIAL_SRC})

target_link_libraries (
    aph-material
    PRIVATE aph-filesystem
            aph-common
            aph-api
            aph-math
            aph-global
            aph-geometry
            aph-reflection
)
