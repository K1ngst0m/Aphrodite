file(GLOB base_src
  common/*.cpp
  common/*.h
  common/*.hpp

  aph_core.hpp
  aph_renderer.hpp

  renderer/*.cpp
  renderer/*.h
  renderer/*.hpp

  renderer/api/vulkan/sceneRenderer.h
  renderer/api/vulkan/renderer.h
  renderer/api/vulkan/uiRenderer.h

  renderer/api/vulkan/sceneRenderer.cpp
  renderer/api/vulkan/renderer.cpp
  renderer/api/vulkan/uiRenderer.cpp

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

add_library(spvgen SHARED IMPORTED)
set_target_properties(spvgen PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/external/libs/spvgen.so"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/external/spvgen/"
)

target_link_libraries(engine
  glfw
  ${xcb_libraries}
  ${cmake_thread_libs_init}
  dl
  spvgen
  )
