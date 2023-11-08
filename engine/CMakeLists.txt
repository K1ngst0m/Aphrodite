add_subdirectory(common)
add_subdirectory(allocator)
add_subdirectory(app)
add_subdirectory(cli)
add_subdirectory(threads)
add_subdirectory(wsi)
add_subdirectory(resource)
add_subdirectory(api)
add_subdirectory(renderer)
add_subdirectory(filesystem)
add_subdirectory(module)
add_subdirectory(renderGraph)

add_library(aph_engine INTERFACE)

target_include_directories(aph_engine INTERFACE
                                  ${APH_EXTERNAL_DIR}/volk
                                  ${APH_EXTERNAL_DIR}/glm
                                  ${APH_ENGINE_DIR}
                                  )
target_link_libraries(aph_engine INTERFACE
  common
  aph_allocator
  threads
  aph_filesystem
  aph_module
  cli
  app
  wsi
  resource
  api
  renderer
  rendergraph
)

target_link_libraries(aph_engine INTERFACE
  mimalloc-static
  ${xcb_libraries}
  ${cmake_thread_libs_init}
  dl
  dw
)
