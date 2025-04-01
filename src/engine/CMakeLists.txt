file (GLOB APH_ENGINE_SRC *.cpp)
aph_setup_target (engine ${APH_ENGINE_SRC})
target_link_libraries (
    aph-engine
    PRIVATE aph-common aph-api imgui
)
