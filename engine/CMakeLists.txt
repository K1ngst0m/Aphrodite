file(GLOB base_src
  common/*.cpp
  common/*.h
  common/*.hpp

  vkl.hpp
  vklHelper.hpp

  renderer/*.cpp
  renderer/*.h
  renderer/*.hpp

  renderer/helper/*.cpp
  renderer/helper/*.h
  renderer/helper/*.hpp

  renderer/api/vulkan/*.cpp
  renderer/api/vulkan/*.h
  renderer/api/vulkan/*.hpp

  scene/*.cpp
  scene/*.h
  scene/*.hpp

  app/*.cpp
  app/*.h
  app/*.hpp

  ../external/spirv_reflect/*.cpp
  ../external/spirv_reflect/*.h
  ../external/imgui/*.cpp
  ../external/imgui/*.h
  ../external/volk/volk.h
  ../external/volk/volk.c
  )


file(GLOB BASE_HEADERS
  vkl.hpp
  vklHelper.hpp

  renderer/*.h
  renderer/*.hpp
  renderer/helper/*.h
  renderer/helper/*.hpp

  scene/*.h
  scene/*.hpp

  ../external/spirv_reflect/*.h
  ../external/imgui/*.h
  )

add_library(engine STATIC ${base_src})

target_link_libraries(engine
  glfw
  ${xcb_libraries}
  ${cmake_thread_libs_init}
  dl
  )
