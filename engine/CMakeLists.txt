set(APH_ENGINE_COMMON_DIR ${APH_ENGINE_DIR}/common)
set(APH_ENGINE_API_DIR ${APH_ENGINE_DIR}/api)
set(APH_ENGINE_RESOURCE_DIR ${APH_ENGINE_DIR}/resource)
set(APH_ENGINE_CLI_DIR ${APH_ENGINE_DIR}/cli)
set(APH_ENGINE_MODULE_DIR ${APH_ENGINE_DIR}/cmodule)
set(APH_ENGINE_APP_DIR ${APH_ENGINE_DIR}/app)
set(APH_ENGINE_RENDERER_DIR ${APH_ENGINE_DIR}/renderer)
set(APH_ENGINE_WSI_DIR ${APH_ENGINE_DIR}/wsi)
set(APH_ENGINE_THREADS_DIR ${APH_ENGINE_DIR}/threads)
# set(APH_ENGINE_SCENE_DIR ${APH_ENGINE_DIR}/scene)
# set(APH_ENGINE_UI_DIR ${APH_ENGINE_DIR}/ui)

file(GLOB ENGINE_BASE_SRC
  # ${APH_ENGINE_MODULE_DIR}/*.cpp
  ${APH_ENGINE_RESOURCE_DIR}/*.cpp

  ${APH_ENGINE_API_DIR}/vulkan/*.cpp

  ${APH_ENGINE_RENDERER_DIR}/api/vulkan/renderer.h
  ${APH_ENGINE_RENDERER_DIR}/api/vulkan/renderer.cpp
  ${APH_ENGINE_RENDERER_DIR}/api/vulkan/uiRenderer.h
  ${APH_ENGINE_RENDERER_DIR}/api/vulkan/uiRenderer.cpp
)

add_subdirectory(common)
add_subdirectory(app)
add_subdirectory(cli)
add_subdirectory(threads)
add_subdirectory(wsi)

add_library(engine STATIC ${ENGINE_BASE_SRC})
aph_compiler_options(engine)

target_include_directories(engine SYSTEM PUBLIC
                                  ${APH_EXTERNAL_DIR}/volk
                                  ${APH_EXTERNAL_DIR}/glm

                                  PRIVATE
                                  ${APH_EXTERNAL_DIR}/tinyimageformat
                                  ${APH_EXTERNAL_DIR}/mmgr
                                  ${APH_EXTERNAL_DIR}/glfw/include
                                  ${APH_EXTERNAL_DIR}/shaderc/libshaderc/include
                                  ${APH_EXTERNAL_DIR}/imgui
                                  ${APH_EXTERNAL_DIR}/vulkan
                                  ${APH_EXTERNAL_DIR}/slang
                                  ${APH_EXTERNAL_DIR}/stb/include
                                  ${APH_EXTERNAL_DIR}/tinygltf/include
                                  ${APH_EXTERNAL_DIR}/tinyktx
                                  ${APH_EXTERNAL_DIR}/tinydds

                                  PUBLIC
                                  ${APH_ENGINE_DIR}
                                  )
target_link_libraries(engine PRIVATE common app cli threads wsi)

target_link_libraries(engine PRIVATE
  mimalloc-static
  glfw
  imgui
  ${xcb_libraries}
  ${cmake_thread_libs_init}
  volk
  shaderc_shared
  spirv-cross-core
  slang
  dl
  dw
)
