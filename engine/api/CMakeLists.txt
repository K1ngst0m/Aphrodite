file(GLOB API_VULKAN_SRC ${APH_ENGINE_API_DIR}/vulkan/*.cpp)
add_library(api STATIC ${API_VULKAN_SRC})
aph_compiler_options(api)

target_include_directories(api PRIVATE
  ${APH_ENGINE_DIR}
)

target_include_directories(api SYSTEM
  PRIVATE
  ${spirv-cross_SOURCE_DIR}
  ${vma_SOURCE_DIR}/include
  PUBLIC
  ${vulkan-headers_SOURCE_DIR}/include
  ${volk_SOURCE_DIR}
  )

target_link_libraries(api PRIVATE
  common
  aph_allocator
  threads
  volk
  spirv-cross-core
  GPUOpen::VulkanMemoryAllocator
)
