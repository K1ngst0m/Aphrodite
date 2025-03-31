file (GLOB APH_SCENE_SRC ${APH_ENGINE_SCENE_DIR}/idObject.h ${APH_ENGINE_SCENE_DIR}/camera.h
      ${APH_ENGINE_SCENE_DIR}/camera.cpp
)
aph_setup_target (scene ${APH_SCENE_SRC})
target_link_libraries (
    aph-scene
    PRIVATE aph-math aph-common
)
