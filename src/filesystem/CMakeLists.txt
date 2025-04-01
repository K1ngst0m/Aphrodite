file (GLOB APH_FILESYSTEM_SRC *.cpp)
aph_setup_target (filesystem ${APH_FILESYSTEM_SRC})
target_link_libraries (
    aph-filesystem
    PRIVATE aph-common
)
