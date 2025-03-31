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
add_subdirectory(scene)
add_subdirectory(module)
add_subdirectory(renderGraph)
add_subdirectory(ui)
add_subdirectory(global)

set(EMPTY_SOURCE_FILES ${CMAKE_CURRENT_BINARY_DIR}/aph.cpp)
add_custom_command(
    OUTPUT  ${EMPTY_SOURCE_FILES}
    COMMAND ${CMAKE_COMMAND} -E touch ${EMPTY_SOURCE_FILES}
    COMMENT "Touching ${EMPTY_SOURCE_FILES}"
)

if (APH_SHARED)
  add_library(aph-engine SHARED ${EMPTY_SOURCE_FILES})
else()
  add_library(aph-engine STATIC ${EMPTY_SOURCE_FILES})
endif()

target_include_directories(aph-engine PUBLIC ${APH_ENGINE_DIR})

target_link_libraries(aph-engine PUBLIC
  aphrodite::common
  aphrodite::math
  aphrodite::allocator
  aphrodite::threads
  aphrodite::filesystem
  aphrodite::global
  aphrodite::module
  aphrodite::ui
  aphrodite::cli
  aphrodite::app
  aphrodite::wsi
  aphrodite::resource
  aphrodite::api
  aphrodite::renderer
  aphrodite::rendergraph
  aphrodite::scene
)

add_library(aphrodite::engine ALIAS aph-engine)

# Add include checking for the engine target
aph_add_include_check_targets(aph-engine)
