file(GLOB API_VULKAN_SRC ${APH_ENGINE_API_DIR}/vulkan/*.cpp)
add_library(api STATIC ${API_VULKAN_SRC})
aph_compiler_options(api)

target_include_directories(api PRIVATE
  ${APH_ENGINE_DIR}
)

target_link_libraries(api
  PUBLIC
  volk
  vulkan-registry

  PRIVATE
  common
  aph_allocator
  threads
  spirv-cross-core
  GPUOpen::VulkanMemoryAllocator
)
