add_library(wsi STATIC ${APH_WSI_SRC})

if(APH_WSI_BACKEND STREQUAL "GLFW")
    add_compile_definitions(WSI_USE_GLFW)
    target_sources(wsi PRIVATE
      ${APH_ENGINE_WSI_DIR}/wsi_glfw.cpp
      ${APH_ENGINE_WSI_DIR}/imgui_impl_glfw.cpp
    )
  target_link_libraries(wsi PRIVATE glfw)
elseif(APH_WSI_BACKEND STREQUAL "SDL2")
    add_compile_definitions(WSI_USE_SDL2)
    target_sources(wsi PRIVATE
      ${APH_ENGINE_WSI_DIR}/wsi_sdl2.cpp
      ${APH_ENGINE_WSI_DIR}/imgui_impl_sdl2.cpp
    )
  target_include_directories(wsi PRIVATE ${SDL2_SOURCE_DIR}/include)
  target_link_libraries(wsi PRIVATE SDL2::SDL2)
endif()

aph_compiler_options(wsi)
target_include_directories(wsi PRIVATE
  ${APH_ENGINE_DIR}
  ${APH_EXTERNAL_DIR}/imgui
)
target_link_libraries(wsi PRIVATE common api imgui)
