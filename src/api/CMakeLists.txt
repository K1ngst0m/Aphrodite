file (GLOB API_VULKAN_SRC *.cpp vulkan/*.cpp)
aph_setup_target (api ${API_VULKAN_SRC})

target_link_libraries (
    aph-api
    PUBLIC vulkan-registry
    PRIVATE aph-common
            aph-wsi
            aph-allocator
            aph-threads
            aph-module
            GPUOpen::VulkanMemoryAllocator
            renderdoc
)
