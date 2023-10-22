file(GLOB base_src
  common/*.cpp
  input/*.cpp
  threads/*.cpp
  cli/*.cpp
  ui/*.cpp
  wsi/*.cpp
  renderer/*.cpp
  api/vulkan/*.cpp
  app/*.cpp
  resource/*.cpp
  renderer/api/vulkan/renderer.cpp

  # scene/*.cpp
  # renderer/api/vulkan/sceneRenderer.cpp
  )

add_library(engine STATIC ${base_src})
aph_compiler_options(engine)

target_include_directories(engine SYSTEM PUBLIC
                                  ../external/glm
                                  ../external/reckless/reckless/include
                                  ../external/tinyimageformat
                                  ../external/volk

                                  PRIVATE
                                  ./
                                  ../external/backward-cpp/include
                                  ../external/mmgr
                                  ../external/glfw/include
                                  ../external/shaderc/libshaderc/include
                                  ../external/imgui/include
                                  ../external/vulkan
                                  ../external/stb/include
                                  ../external/tinygltf/include
                                  ../external/spirv-cross
                                  ../external/tinyktx
                                  ../external/tinydds
                                  )

target_link_libraries(engine PRIVATE
  mimalloc-static
  glfw
  ${xcb_libraries}
  ${cmake_thread_libs_init}
  volk
  reckless
  imgui
  shaderc_shared
  spirv-cross-core
  dl
  dw
  # mmgr
  # spirv-cross-reflect
)
