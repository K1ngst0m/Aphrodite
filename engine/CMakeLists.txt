file(GLOB base_src
  common/*.cpp
  common/*.h
  common/*.hpp

  vkl.hpp

  renderer/*.cpp
  renderer/*.h
  renderer/*.hpp

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
  )


file(GLOB BASE_HEADERS
  vkl.hpp

  renderer/*.h
  renderer/*.hpp

  scene/*.h
  scene/*.hpp

  ../external/spirv_reflect/*.h
  ../external/imgui/*.h
  )

add_library(engine STATIC ${base_src})

target_link_libraries(engine ${vulkan_lib} glfw ${xcb_libraries} ${cmake_thread_libs_init})
