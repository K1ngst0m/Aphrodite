file(GLOB base_src
  common/*.cpp
  common/*.h
  common/*.hpp

  vkl.hpp
  vklRenderer.hpp

  renderer/*.cpp
  renderer/*.h
  renderer/*.hpp

  renderer/api/vulkan/sceneRenderer.h
  renderer/api/vulkan/renderer.h
  # renderer/api/vulkan/uiRenderer.cpp

  renderer/api/vulkan/sceneRenderer.cpp
  renderer/api/vulkan/renderer.cpp
  # renderer/api/vulkan/uiRenderer.cpp

  api/vulkan/*.cpp
  api/vulkan/*.h
  api/vulkan/*.hpp

  scene/*.cpp
  scene/*.h
  scene/*.hpp

  app/*.cpp
  app/*.h
  app/*.hpp

  # ../external/spirv_reflect/*.cpp
  # ../external/spirv_reflect/*.h
  # ../external/imgui/*.cpp
  # ../external/imgui/*.h
  ../external/volk/volk.h
  ../external/volk/volk.c
  )

add_library(engine STATIC ${base_src})

target_link_libraries(engine
  glfw
  ${xcb_libraries}
  ${cmake_thread_libs_init}
  dl
  )
