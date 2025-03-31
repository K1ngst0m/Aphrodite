aph_setup_target (wsi ${APH_WSI_SRC} wsi.cpp)

target_compile_definitions (
    aph-wsi
    PUBLIC $<$<BOOL:${APH_WSI_BACKEND_IS_SDL}>:WSI_USE_SDL>
)
target_link_libraries (
    aph-wsi
    PRIVATE aph-common aph-api imgui $<$<BOOL:${APH_WSI_BACKEND_IS_SDL}>:SDL3::SDL3-static>
)
