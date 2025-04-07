file (GLOB APH_RDG_SRC *.cpp)
aph_setup_target (rendergraph ${APH_RDG_SRC})
target_link_libraries (
    aph-rendergraph
    PRIVATE aph-common aph-api
)
