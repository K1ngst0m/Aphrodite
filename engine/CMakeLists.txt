file(GLOB base_src
  common/*.cpp
  common/*.h
  common/*.hpp

  aph_core.hpp
  aph_renderer.hpp

  input/*.cpp
  input/*.h
  input/*.hpp

  threads/*.cpp
  threads/*.h
  threads/*.hpp

  cli/*.cpp
  cli/*.h
  cli/*.hpp

  ui/*.cpp
  ui/*.h
  ui/*.hpp

  wsi/*.cpp
  wsi/*.h
  wsi/*.hpp

  renderer/*.cpp
  renderer/*.h
  renderer/*.hpp

  renderer/api/vulkan/sceneRenderer.h
  renderer/api/vulkan/renderer.h

  renderer/api/vulkan/sceneRenderer.cpp
  renderer/api/vulkan/renderer.cpp

  api/vulkan/*.cpp
  api/vulkan/*.h
  api/vulkan/*.hpp

  scene/*.cpp
  scene/*.h
  scene/*.hpp

  app/*.cpp
  app/*.h
  app/*.hpp

  ${CMAKE_SOURCE_DIR}/external/imgui/*.cpp
  ${CMAKE_SOURCE_DIR}/external/imgui/*.h

  ${CMAKE_SOURCE_DIR}/external/volk/volk.h
  ${CMAKE_SOURCE_DIR}/external/volk/volk.c
  )

add_library(engine STATIC ${base_src})

target_link_libraries(engine
  glfw
  ${xcb_libraries}
  ${cmake_thread_libs_init}
  dl
  reckless
  )
