file (GLOB APH_SCENE_SRC *.cpp)
aph_setup_target (scene ${APH_SCENE_SRC})
target_link_libraries (
    aph-scene
    PRIVATE aph-math aph-common
)
