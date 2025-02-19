file(GLOB APH_RDG_SRC
  ${APH_ENGINE_RDG_DIR}/*.cpp
)
aph_setup_target(rendergraph ${APH_RDG_SRC})
target_link_libraries(aph-rendergraph PRIVATE aph-common aph-api)
