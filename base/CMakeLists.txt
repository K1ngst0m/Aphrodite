file(GLOB base_src
  *.cpp
  *.h
  *.hpp
  ../external/spirv_reflect/*.cpp
  ../external/spirv_reflect/*.h
  ../external/imgui/*.cpp
  ../external/imgui/*.h
  )


file(GLOB BASE_HEADERS
  *.h
  *.hpp
  ../external/spirv_reflect/*.h
  ../external/imgui/*.h
  )

add_library(base STATIC ${base_src})

target_link_libraries(base ${vulkan_lib} glfw ${xcb_libraries} ${cmake_thread_libs_init})
