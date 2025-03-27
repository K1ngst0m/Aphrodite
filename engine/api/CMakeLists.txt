file(GLOB API_VULKAN_SRC ${APH_ENGINE_API_DIR}/vulkan/*.cpp ${APH_ENGINE_API_DIR}/vulkan/memory/*.cpp)
aph_setup_target(api ${API_VULKAN_SRC})

target_link_libraries(aph-api
  PUBLIC
  vulkan-registry

  PRIVATE
  aph-common
  aph-allocator
  aph-threads

  GPUOpen::VulkanMemoryAllocator
  renderdoc
)
