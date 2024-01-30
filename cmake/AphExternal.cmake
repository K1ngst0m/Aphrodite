include(CPM)

set(VK_SDK_VERSION 1.3.275)

CPMAddPackage(
  NAME tinygltf
  GITHUB_REPOSITORY syoyo/tinygltf
  VERSION 2.8.18
  DOWNLOAD_ONLY True
)

CPMAddPackage(
  NAME tracy
  GITHUB_REPOSITORY wolfpld/tracy
  VERSION 0.10
)

CPMAddPackage(
  NAME glm
  GITHUB_REPOSITORY g-truc/glm
  GIT_TAG 0.9.9.8
  OPTIONS
      "GLM_TEST_ENABLE OFF"
)

CPMAddPackage(
  NAME unordered_dense
  GITHUB_REPOSITORY martinus/unordered_dense
  VERSION 4.1.2
)

CPMAddPackage(
  NAME vma
  GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
  GIT_TAG master
)

CPMAddPackage(
  NAME mimalloc
  GITHUB_REPOSITORY microsoft/mimalloc
  VERSION 2.1.2
  OPTIONS
      "MI_BUILD_SHARED OFF"
      "MI_BUILD_OBJECT OFF"
      "MI_BUILD_STATIC ON"
      "MI_BUILD_TESTS OFF"
      "MI_USE_CXX ON"
      "MI_OVERRIDE ON"
)

CPMAddPackage(
  NAME slang
  URL https://github.com/shader-slang/slang/releases/download/v2024.0.0/slang-2024.0.0-linux-x86_64.tar.gz
  VERSION 2024.0.0
  DOWNLOAD_ONLY YES
)

CPMAddPackage(
  NAME spirv-cross
  GITHUB_REPOSITORY KhronosGroup/SPIRV-Cross
  GIT_TAG vulkan-sdk-${VK_SDK_VERSION}
  OPTIONS
      "SPIRV_CROSS_CLI OFF"
      "SPIRV_CROSS_ENABLE_TESTS ON"
)

CPMAddPackage(
  NAME vulkan-headers
  GITHUB_REPOSITORY KhronosGroup/Vulkan-Headers
  GIT_TAG vulkan-sdk-${VK_SDK_VERSION}
  DOWNLOAD_ONLY YES
)

CPMAddPackage(
  NAME volk
  GITHUB_REPOSITORY zeux/volk
  GIT_TAG vulkan-sdk-${VK_SDK_VERSION}
)

add_subdirectory(${APH_EXTERNAL_DIR}/imgui EXCLUDE_FROM_ALL)

# wsi backend
set(VALID_WSI_BACKENDS Auto GLFW SDL2)
if(NOT (APH_WSI_BACKEND IN_LIST VALID_WSI_BACKENDS))
    message(FATAL_ERROR "Wrong value passed for APH_WSI_BACKEND, use one of: Auto, GLFW, SDL2")
endif()

if(APH_WSI_BACKEND STREQUAL "Auto" OR APH_WSI_BACKEND STREQUAL "GLFW")
    CPMAddPackage(
            NAME GLFW
            GITHUB_REPOSITORY glfw/glfw
            GIT_TAG 3.3.9
            OPTIONS
                "GLFW_BUILD_TESTS OFF"
                "GLFW_BUILD_EXAMPLES OFF"
                "GLFW_BULID_DOCS OFF"
                "GLFW_INSTALL OFF"
    )
    if(NOT GLFW_ADDED)
        message(FATAL_ERROR "GLFW3 library not found!")
    endif()
elseif(APH_WSI_BACKEND STREQUAL "SDL2")
    CPMAddPackage(
      NAME SDL2
      VERSION 2.29.3
      URL https://libsdl.org/release/SDL2-2.29.3.zip
    )
    if(NOT SDL2_ADDED)
        message(FATAL_ERROR "SDL2 library not found!")
    endif()
endif()
