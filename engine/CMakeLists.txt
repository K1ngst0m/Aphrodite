file(GLOB base_src
  common/*.cpp
  input/*.cpp
  threads/*.cpp
  cli/*.cpp
  # ui/*.cpp
  wsi/*.cpp
  renderer/*.cpp
  api/vulkan/*.cpp
  app/*.cpp
  resource/*.cpp
  renderer/api/vulkan/renderer.h
  renderer/api/vulkan/renderer.cpp

  renderer/api/vulkan/uiRenderer.h
  renderer/api/vulkan/uiRenderer.cpp

)

add_library(engine STATIC ${base_src})
aph_compiler_options(engine)

target_include_directories(engine SYSTEM PUBLIC
                                  ${APH_EXTERNAL_DIR}/glm
                                  ${APH_EXTERNAL_DIR}/reckless/reckless/include
                                  ${APH_EXTERNAL_DIR}/tinyimageformat
                                  ${APH_EXTERNAL_DIR}/volk

                                  PRIVATE
                                  ${APH_ENGINE_DIR}
                                  ${APH_EXTERNAL_DIR}/backward-cpp/include
                                  ${APH_EXTERNAL_DIR}/mmgr
                                  ${APH_EXTERNAL_DIR}/glfw/include
                                  ${APH_EXTERNAL_DIR}/shaderc/libshaderc/include
                                  ${APH_EXTERNAL_DIR}/imgui
                                  ${APH_EXTERNAL_DIR}/vulkan
                                  ${APH_EXTERNAL_DIR}/stb/include
                                  ${APH_EXTERNAL_DIR}/tinygltf/include
                                  ${APH_EXTERNAL_DIR}/spirv-cross
                                  ${APH_EXTERNAL_DIR}/tinyktx
                                  ${APH_EXTERNAL_DIR}/tinydds
                                  )

target_link_libraries(engine PRIVATE
  mimalloc-static
  glfw
  imgui
  ${xcb_libraries}
  ${cmake_thread_libs_init}
  volk
  reckless
  # imgui
  shaderc_shared
  spirv-cross-core
  dl
  dw
  # mmgr
  # spirv-cross-reflect
)
