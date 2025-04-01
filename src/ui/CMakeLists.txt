file (GLOB APH_UI_SRC *.cpp *.h)

aph_setup_target (ui ${APH_UI_SRC})

target_compile_definitions (
    aph-ui
    PUBLIC $<$<BOOL:${APH_WSI_BACKEND_IS_SDL}>:WSI_USE_SDL>
)

target_link_libraries (
    aph-ui
    PUBLIC imgui
    PRIVATE aph-math aph-common aph-api
)
