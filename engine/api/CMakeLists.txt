file(GLOB API_VULKAN_SRC ${APH_ENGINE_API_DIR}/vulkan/*.cpp)
add_library(api STATIC ${API_VULKAN_SRC})
aph_compiler_options(api)
target_include_directories(api PRIVATE
  ${APH_ENGINE_DIR}
  ${APH_EXTERNAL_DIR}/spirv-cross
  ${APH_EXTERNAL_DIR}/vulkan
  ${APH_EXTERNAL_DIR}/vma/include

  PUBLIC
  ${APH_EXTERNAL_DIR}/volk
)
target_link_libraries(api PRIVATE
  common
  aph_allocator
  threads
  volk
  spirv-cross-core
  GPUOpen::VulkanMemoryAllocator
)
