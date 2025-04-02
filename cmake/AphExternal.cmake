include(CPM)

set(VK_SDK_VERSION 1.4.309.0)
set(APH_PATCH_DIR ${CMAKE_SOURCE_DIR}/patches)

CPMAddPackage(
  NAME Format.cmake
  VERSION 1.8.3
  GITHUB_REPOSITORY TheLartians/Format.cmake
  OPTIONS
      # set to yes skip cmake formatting
      "FORMAT_SKIP_CMAKE YES"
      # set to yes skip clang formatting
      "FORMAT_SKIP_CLANG NO"
      # path to exclude (optional, supports regular expressions)
      "CMAKE_FORMAT_EXCLUDE cmake/CPM.cmake"
)

CPMAddPackage(
  NAME tinygltf
  GITHUB_REPOSITORY syoyo/tinygltf
  VERSION 2.8.18
)

if (APH_ENABLE_TRACING)
CPMAddPackage(
  NAME tracy
  GITHUB_REPOSITORY wolfpld/tracy
  VERSION 0.11.1
)
endif()

CPMAddPackage(
  NAME glm
  GITHUB_REPOSITORY g-truc/glm
  GIT_TAG 1.0.1
  OPTIONS
      "GLM_TEST_ENABLE OFF"
)
target_include_directories(glm PUBLIC ${glm_SOURCE_DIR})

CPMAddPackage(
  NAME tomlplusplus
  GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
  GIT_TAG        v3.4.0
)

CPMAddPackage(
  NAME unordered_dense
  GITHUB_REPOSITORY martinus/unordered_dense
  VERSION 4.1.2
)

CPMAddPackage(
  NAME libcoro
  GITHUB_REPOSITORY jbaldwin/libcoro
  GIT_TAG v0.14.1
)

CPMAddPackage(
  NAME backward-cpp
  GITHUB_REPOSITORY bombela/backward-cpp
  GIT_TAG master
)

CPMAddPackage(
  NAME vma
  GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
  GIT_TAG master
)

CPMAddPackage(
  NAME mimalloc
  GITHUB_REPOSITORY microsoft/mimalloc
  VERSION 3.0.1
  OPTIONS
      "MI_BUILD_SHARED OFF"
      "MI_BUILD_OBJECT OFF"
      "MI_BUILD_STATIC ON"
      "MI_BUILD_TESTS OFF"
      "MI_USE_CXX ON"
      "MI_OVERRIDE ON"
)

CPMAddPackage(
  NAME stb
  GITHUB_REPOSITORY nothings/stb
  GIT_TAG master
)
add_library(stb INTERFACE IMPORTED)
target_include_directories(stb SYSTEM INTERFACE ${stb_SOURCE_DIR})

CPMAddPackage(
  NAME slang
  URL https://github.com/shader-slang/slang/releases/download/vulkan-sdk-${VK_SDK_VERSION}/slang-ulkan-sdk-${VK_SDK_VERSION}-linux-x86_64.tar.gz
  VERSION ${VK_SDK_VERSION}
  DOWNLOAD_ONLY YES
)
add_library(slang SHARED IMPORTED)
set_property(TARGET slang PROPERTY IMPORTED_LOCATION ${slang_SOURCE_DIR}/lib/libslang.so)
target_include_directories(slang INTERFACE ${slang_SOURCE_DIR}/include)
target_link_libraries(slang INTERFACE ${slang_SOURCE_DIR}/lib/libslang.so)


CPMAddPackage(
  NAME spirv-cross
  GITHUB_REPOSITORY KhronosGroup/SPIRV-Cross
  GIT_TAG vulkan-sdk-${VK_SDK_VERSION}
  OPTIONS
      "SPIRV_CROSS_CLI OFF"
      "SPIRV_CROSS_ENABLE_TESTS OFF"
      "SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS ON"
)

CPMAddPackage(
  NAME vulkan-headers
  GITHUB_REPOSITORY KhronosGroup/Vulkan-Headers
  GIT_TAG vulkan-sdk-${VK_SDK_VERSION}
  DOWNLOAD_ONLY YES
)
add_library(vulkan-registry INTERFACE IMPORTED)
target_include_directories(vulkan-registry INTERFACE ${vulkan-headers_SOURCE_DIR}/include)
target_compile_definitions(vulkan-registry INTERFACE
  VULKAN_HPP_NO_EXCEPTIONS
  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)

# wsi backend
set(VALID_WSI_BACKENDS Auto SDL)
if(NOT (APH_WSI_BACKEND IN_LIST VALID_WSI_BACKENDS))
    message(FATAL_ERROR "Wrong value passed for APH_WSI_BACKEND, use one of: Auto, SDL")
endif()

if(APH_WSI_BACKEND STREQUAL "Auto")
  message("WSI backend is set to 'Auto', choose the SDL by default")
  set(APH_WSI_BACKEND "SDL")
endif()

if(APH_WSI_BACKEND STREQUAL "SDL")
  set(APH_WSI_BACKEND_IS_SDL "ON")
    CPMAddPackage(
      NAME SDL
      VERSION 3.2.8
      URL https://github.com/libsdl-org/SDL/releases/download/release-3.2.8/SDL3-3.2.8.tar.gz
      OPTIONS
          "SDL_SHARED OFF"
          "SDL_STATIC ON"
          "SDL_TEST OFF"
    )
    if(NOT SDL_ADDED)
        message(FATAL_ERROR "SDL library not found!")
    endif()
endif()


CPMAddPackage(
  NAME renderdoc
  VERSION 1.36
  URL https://renderdoc.org/stable/1.36/renderdoc_1.36.tar.gz
  DOWNLOAD_ONLY YES
)
add_library(renderdoc INTERFACE)
target_include_directories(renderdoc INTERFACE ${renderdoc_SOURCE_DIR}/include/)

CPMAddPackage(
  NAME imgui
  VERSION 1.91.8
  URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.8-docking.tar.gz
  DOWNLOAD_ONLY YES
)

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui
  PRIVATE
  SDL3::SDL3-static
  vulkan-registry
)
target_compile_definitions(imgui PRIVATE VK_NO_PROTOTYPES)
