if(APH_WSI_BACKEND STREQUAL "Auto" OR APH_WSI_BACKEND STREQUAL "GLFW")
    add_compile_definitions(WSI_USE_GLFW)
    set(APH_WSI_SRC ${APH_ENGINE_WSI_DIR}/wsi_glfw.cpp)
elseif(APH_WSI_BACKEND STREQUAL "SDL2")
    if(NOT SDL2_FOUND)
        message(FATAL_ERROR "SDL2 library not found!")
    endif()
    add_compile_definitions(WSI_USE_SDL2)
    message(FATAL_ERROR "SDL2 is not supported.")
    # set(APH_WSI_SRC ${APH_ENGINE_WSI_DIR}/wsi_sdl2.cpp)
endif()

add_library(wsi STATIC ${APH_WSI_SRC})
aph_compiler_options(wsi)
target_include_directories(wsi PRIVATE
  ${APH_ENGINE_DIR}
  ${APH_EXTERNAL_DIR}/imgui
)

target_link_libraries(wsi PRIVATE common api imgui glfw)
