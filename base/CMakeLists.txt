file(GLOB base_src
  *.cpp
  *.h
  *.hpp
  )

file(GLOB BASE_HEADERS
  *.h
  *.hpp
  )

add_library(base STATIC ${base_src})

target_link_libraries(base -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi)
