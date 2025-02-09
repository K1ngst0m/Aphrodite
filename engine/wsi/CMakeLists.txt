add_library(wsi STATIC ${APH_WSI_SRC})

aph_compiler_options(wsi)
target_include_directories(wsi PRIVATE ${APH_ENGINE_DIR})
target_compile_definitions(wsi PUBLIC
  $<$<BOOL:${APH_WSI_BACKEND_IS_GLFW}>:WSI_USE_GLFW>
  $<$<BOOL:${APH_WSI_BACKEND_IS_SDL2}>:WSI_USE_SDL2>
)
target_sources(wsi PRIVATE
  $<$<BOOL:${APH_WSI_BACKEND_IS_GLFW}>:${APH_ENGINE_WSI_DIR}/wsi_glfw.cpp>
  $<$<BOOL:${APH_WSI_BACKEND_IS_SDL2}>:${APH_ENGINE_WSI_DIR}/wsi_sdl2.cpp>
)
target_link_libraries(wsi
  PRIVATE
  common api imgui
  $<$<BOOL:${APH_WSI_BACKEND_IS_GLFW}>:glfw>
  $<$<BOOL:${APH_WSI_BACKEND_IS_SDL2}>:SDL2::SDL2-static>
)
