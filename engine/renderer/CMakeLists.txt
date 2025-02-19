file(GLOB RENDERER_VULKAN_SRC
  ${APH_ENGINE_RENDERER_DIR}/uiRenderer.cpp
  ${APH_ENGINE_RENDERER_DIR}/renderer.cpp
)
aph_setup_target(renderer ${RENDERER_VULKAN_SRC})
target_link_libraries(aph-renderer PRIVATE aph-common aph-api imgui)
