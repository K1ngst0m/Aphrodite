aph_setup_target (wsi ${APH_WSI_SRC})

target_compile_definitions (
    aph-wsi
    PUBLIC $<$<BOOL:${APH_WSI_BACKEND_IS_GLFW}>:WSI_USE_GLFW>
           $<$<BOOL:${APH_WSI_BACKEND_IS_SDL}>:WSI_USE_SDL>
)
target_sources (
    aph-wsi
    PRIVATE $<$<BOOL:${APH_WSI_BACKEND_IS_GLFW}>:${APH_ENGINE_WSI_DIR}/wsi_glfw.cpp>
            $<$<BOOL:${APH_WSI_BACKEND_IS_SDL}>:${APH_ENGINE_WSI_DIR}/wsi_sdl.cpp>
)
target_link_libraries (
    aph-wsi
    PRIVATE aph-common aph-api imgui $<$<BOOL:${APH_WSI_BACKEND_IS_GLFW}>:glfw>
            $<$<BOOL:${APH_WSI_BACKEND_IS_SDL}>:SDL3::SDL3-static>
)
