file (GLOB APH_FILESYSTEM_SRC ${APH_ENGINE_FILESYSTEM_DIR}/*.cpp)
aph_setup_target (filesystem ${APH_FILESYSTEM_SRC})
target_link_libraries (
    aph-filesystem
    PRIVATE aph-common
)
