file(GLOB RENDERER_VULKAN_SRC
  ${APH_ENGINE_RENDERER_DIR}/uiRenderer.cpp
  ${APH_ENGINE_RENDERER_DIR}/renderer.cpp
)
add_library(renderer STATIC ${RENDERER_VULKAN_SRC})
aph_compiler_options(renderer)
target_include_directories(renderer PRIVATE ${APH_ENGINE_DIR})
target_link_libraries(renderer PRIVATE common api imgui)
