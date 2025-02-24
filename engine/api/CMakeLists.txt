file(GLOB API_VULKAN_SRC ${APH_ENGINE_API_DIR}/vulkan/*.cpp)
aph_setup_target(api ${API_VULKAN_SRC})

target_link_libraries(aph-api
  PUBLIC
  volk

  PRIVATE
  aph-common
  aph-allocator
  aph-threads

  vulkan-registry
  GPUOpen::VulkanMemoryAllocator
  renderdoc
)
