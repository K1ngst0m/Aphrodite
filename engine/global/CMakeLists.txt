file (GLOB APH_GLOBAL_SRC ${APH_ENGINE_GLOBAL_DIR}/*.cpp)
aph_setup_target (global ${APH_GLOBAL_SRC})
target_link_libraries (
    aph-global
    PRIVATE aph-threads aph-common
)
