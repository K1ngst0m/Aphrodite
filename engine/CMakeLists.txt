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

add_subdirectory(common)
add_subdirectory(app)
add_subdirectory(cli)
add_subdirectory(threads)
add_subdirectory(wsi)
add_subdirectory(resource)
add_subdirectory(api)
add_subdirectory(renderer)

add_library(engine INTERFACE)

target_include_directories(engine INTERFACE
                                  ${APH_EXTERNAL_DIR}/volk
                                  ${APH_EXTERNAL_DIR}/glm
                                  ${APH_ENGINE_DIR}
                                  )
target_link_libraries(engine INTERFACE common app cli threads wsi resource api renderer)

target_link_libraries(engine INTERFACE
  mimalloc-static
  glfw
  imgui
  ${xcb_libraries}
  ${cmake_thread_libs_init}
  volk
  shaderc_shared
  spirv-cross-core
  dl
  dw
)
