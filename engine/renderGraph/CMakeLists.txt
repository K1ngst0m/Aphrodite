file(GLOB APH_RDG_SRC
  ${APH_ENGINE_RDG_DIR}/*.cpp
)
add_library(rendergraph STATIC ${APH_RDG_SRC})
aph_compiler_options(rendergraph)
target_include_directories(rendergraph PRIVATE
  ${APH_ENGINE_DIR}
)
target_link_libraries(rendergraph PRIVATE common api)
