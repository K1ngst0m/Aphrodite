add_subdirectory(common)
add_subdirectory(event)
add_subdirectory(math)
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
add_subdirectory(ui)

add_library(aph_engine INTERFACE)

target_include_directories(aph_engine INTERFACE
                                  ${APH_ENGINE_DIR}
                                  SYSTEM INTERFACE
                                  ${APH_EXTERNAL_DIR}/volk
                                  ${unordered_dense_SOURCE_DIR}/include
                                  )
target_link_libraries(aph_engine INTERFACE
  common
  aph_math
  aph_allocator
  threads
  aph_filesystem
  aph_module
  aph_ui
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
  dl
  dw
)
