file(GLOB base_src
  common/*.cpp
  input/*.cpp
  threads/*.cpp
  cli/*.cpp
  ui/*.cpp
  wsi/*.cpp
  renderer/*.cpp
  api/vulkan/*.cpp
  scene/*.cpp
  app/*.cpp
  renderer/api/vulkan/sceneRenderer.cpp
  renderer/api/vulkan/renderer.cpp
  )

add_library(engine STATIC ${base_src})

target_include_directories(engine PUBLIC  ../external/glm
                                  PUBLIC  ../external/volk
                                  PUBLIC  ../external/reckless/reckless/include
                                  PRIVATE ../external/libshaderc/include
                                  PRIVATE ../external/glfw/include
                                  PRIVATE ../external/imgui/include
                                  PRIVATE ../external/vulkan
                                  PRIVATE ../external/stb/include
                                  PRIVATE ../external/tinygltf/include
                                  )

target_link_libraries(engine
  glfw
  ${xcb_libraries}
  ${cmake_thread_libs_init}
  volk
  dl
  reckless
  imgui
  shaderc_shared
)
