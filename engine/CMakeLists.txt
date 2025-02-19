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

add_library(aph-engine INTERFACE)
target_include_directories(aph-engine INTERFACE ${APH_ENGINE_DIR})

target_link_libraries(aph-engine INTERFACE
  aphrodite::common
  aphrodite::math
  aphrodite::allocator
  aphrodite::threads
  aphrodite::filesystem
  aphrodite::module
  aphrodite::ui
  aphrodite::cli
  aphrodite::app
  aphrodite::wsi
  aphrodite::resource
  aphrodite::api
  aphrodite::renderer
  aphrodite::rendergraph
)

add_library(aphrodite::engine ALIAS aph-engine)
